#include "World/MELamp.h"
#include "AI/MENoise.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "MELamp"

AMELamp::AMELamp()
{
	// Only ticks while flickering
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
}

void AMELamp::BeginPlay()
{
	Super::BeginPlay();

	// Every light the BP added, at the intensity set there
	TArray<ULightComponent*> Found;
	GetComponents(Found);
	for (ULightComponent* Light : Found)
	{
		Lights.Add(Light);
		LightIntensities.Add(Light->Intensity);
	}

	for (int32 Index = 0; Index < Mesh->GetNumMaterials(); ++Index)
	{
		if (UMaterialInstanceDynamic* Material = Mesh->CreateAndSetMaterialInstanceDynamic(Index))
		{
			GlowMaterials.Add(Material);
		}
	}

	State = bStartsOn ? EMELampState::On : EMELampState::Off;
	ApplyState();
}

void AMELamp::ApplyBrightness(float Scale)
{
	for (int32 Index = 0; Index < Lights.Num(); ++Index)
	{
		if (ULightComponent* Light = Lights[Index])
		{
			Light->SetIntensity(LightIntensities[Index] * Scale);
			Light->SetVisibility(Scale > 0.f);
		}
	}
	for (UMaterialInstanceDynamic* Material : GlowMaterials)
	{
		Material->SetScalarParameterValue(GlowParameter, GlowOn * Scale);
	}
}

void AMELamp::ApplyState()
{
	const bool bLit = State == EMELampState::On;
	if (FlickerRemaining <= 0.f)
	{
		ApplyBrightness(bLit ? 1.f : 0.f);
	}

	if (bLit && HumSound && !Hum)
	{
		Hum = UGameplayStatics::SpawnSoundAttached(HumSound, Mesh);
	}
	else if (!bLit && Hum)
	{
		Hum->Stop();
		Hum = nullptr;
	}

	if (bLit && bFlickers)
	{
		ScheduleIdleFlicker();
	}
	else
	{
		GetWorldTimerManager().ClearTimer(IdleFlickerTimer);
	}
}

void AMELamp::SetOn(bool bOn)
{
	// A switch starting before the lamp: the lamp starts in the switch's state
	if (!HasActorBegunPlay())
	{
		bStartsOn = bOn;
		return;
	}
	if (State == EMELampState::Broken || bOn == IsLit())
	{
		return;
	}
	State = bOn ? EMELampState::On : EMELampState::Off;
	ApplyState();
}

void AMELamp::Break(AActor* Breaker)
{
	if (State == EMELampState::Broken)
	{
		return;
	}

	const bool bWasLit = IsLit();
	State = EMELampState::Broken;
	ApplyState();

	const FVector Center = Mesh->Bounds.Origin;
	if (BreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BreakSound, Center);
	}
	if (BreakEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BreakEffect, Center);
	}
	// Glass falling: heard, but it does not tell who
	MENoise::Report(Breaker ? Breaker : this, Center, BreakNoise, MENoise::Glass);

	// A lit bulb dies sputtering, an unlit one just breaks
	if (bWasLit && BreakFlickerTime > 0.f)
	{
		StartFlicker(BreakFlickerTime);
	}
}

float AMELamp::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float Damage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (bCanBreak && DamageAmount >= BreakDamage)
	{
		Break(EventInstigator ? EventInstigator->GetPawn() : DamageCauser);
	}
	return Damage;
}

void AMELamp::StartFlicker(float Duration)
{
	FlickerRemaining = Duration;
	NextFlickerJump = 0.f;
	SetActorTickEnabled(true);
}

void AMELamp::ScheduleIdleFlicker()
{
	const float Delay = FMath::FRandRange(FlickerInterval.X, FMath::Max(FlickerInterval.X, FlickerInterval.Y));
	GetWorldTimerManager().SetTimer(IdleFlickerTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (IsLit())
		{
			StartFlicker(FlickerTime);
			ScheduleIdleFlicker();
		}
	}), Delay, false);
}

void AMELamp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FlickerRemaining -= DeltaSeconds;
	if (FlickerRemaining <= 0.f)
	{
		FlickerRemaining = 0.f;
		SetActorTickEnabled(false);
		ApplyBrightness(IsLit() ? 1.f : 0.f);
		return;
	}

	NextFlickerJump -= DeltaSeconds;
	if (NextFlickerJump > 0.f)
	{
		return;
	}
	NextFlickerJump = 1.f / FlickerRate;

	// Sputters between dark and bright, often out
	ApplyBrightness(FMath::FRand() < 0.45f ? 0.f : FMath::FRandRange(0.3f, 1.f));
}

bool AMELamp::CanBeFocused(const AMECharacter* Interactor) const
{
	return bCanToggleByHand && State != EMELampState::Broken;
}

FMEInteractionInfo AMELamp::GetInteractionInfo(const AMECharacter* Interactor) const
{
	FMEInteractionInfo Info;
	Info.Gesture = EMEInteractionGesture::Switch;
	Info.ActionText = IsLit() ? LOCTEXT("TurnOff", "Turn off") : LOCTEXT("TurnOn", "Turn on");
	return Info;
}

void AMELamp::Interact(AMECharacter* Interactor)
{
	SetOn(!IsLit());
	if (ToggleSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ToggleSound, Mesh->Bounds.Origin);
	}
}

FVector AMELamp::GetGroundLocation() const
{
	const FVector Start = Mesh->Bounds.Origin;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MELampGround), false, this);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - FVector(0.f, 0.f, 2000.f), ECC_Visibility, Params))
	{
		return Hit.ImpactPoint;
	}
	return Start;
}

#undef LOCTEXT_NAMESPACE
