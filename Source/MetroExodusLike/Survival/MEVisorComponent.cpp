#include "Survival/MEVisorComponent.h"
#include "Player/MECharacter.h"
#include "Player/MEFirstPersonCameraComponent.h"
#include "Weapons/MEWeaponComponent.h"
#include "Combat/MEHealthComponent.h"
#include "Combat/MEDamageType_Toxic.h"
#include "Survival/MEGasMaskComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UMEVisorComponent::UMEVisorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMEVisorComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AMECharacter>(GetOwner());
	if (!Character || !VisorMaterial)
	{
		SetComponentTickEnabled(false);
		return;
	}

	Material = UMaterialInstanceDynamic::Create(VisorMaterial, this);
	Character->GetFirstPersonCamera()->PostProcessSettings.WeightedBlendables.Array.Add(FWeightedBlendable(0.f, Material));
	Character->GetHealth()->OnHealthChanged.AddUObject(this, &UMEVisorComponent::HandleHealthChanged);
}

void UMEVisorComponent::HandleHealthChanged(float Health, float Delta, AController* Instigator)
{
	const UDamageType* DamageType = Character->GetHealth()->GetCurrentDamageType();
	if (-Delta < CrackDamage || IsBroken() || !Character->GetGasMask()->IsWorn() || (DamageType && DamageType->IsA<UMEDamageType_Toxic>()))
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastCrackTime < CrackCooldown)
	{
		return;
	}
	LastCrackTime = Now;

	// Each crack somewhere on the visor, away from the very center so the view stays usable
	const FVector2D Direction = FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)).GetSafeNormal();
	const FVector2D Position = Direction * FVector2D(FMath::FRandRange(0.15f, 0.5f), FMath::FRandRange(0.1f, 0.3f));
	if (Material)
	{
		static const FName CrackParams[] = { TEXT("CrackA"), TEXT("CrackB"), TEXT("CrackC") };
		Material->SetVectorParameterValue(CrackParams[FMath::Min(Cracks, 2)], FLinearColor(Position.X, Position.Y, FMath::FRand() * 100.f, 0.f));
	}
	++Cracks;

	USoundBase* Sound = IsBroken() && BreakSound ? BreakSound.Get() : CrackSound.Get();
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

void UMEVisorComponent::Wipe()
{
	if (bWiping || !Character || Character->GetGasMask()->GetState() != EMEGasMaskState::On)
	{
		return;
	}

	const float WeaponAway = Character->GetWeapons()->PutAwayFor(WipeTime);
	if (WeaponAway < 0.f)
	{
		return;
	}

	bWiping = true;
	GetWorld()->GetTimerManager().SetTimer(WipeTimer, this, &UMEVisorComponent::StartWipe, FMath::Max(WeaponAway, 0.01f));
}

void UMEVisorComponent::StartWipe()
{
	WipeProgress = KINDA_SMALL_NUMBER;
	if (WipeSound)
	{
		UGameplayStatics::PlaySound2D(this, WipeSound);
	}
}

void UMEVisorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Ticks all the time: a handful of values, and the mask can go on at any moment
	const bool bMaskOn = Character->GetGasMask()->IsWorn() && !Character->GetHealth()->IsDead();
	MaskAlpha = FMath::FInterpConstantTo(MaskAlpha, bMaskOn ? 1.f : 0.f, DeltaTime, 1.f / MaskFadeTime);

	// Breathing fogs the visor, more out of breath; it clears slowly by itself
	const float TargetFog = bMaskOn ? FMath::Lerp(RestedFog, ExhaustedFog, Character->GetFirstPersonCamera()->GetExertion()) : 0.f;
	Fog = TargetFog > Fog
		? FMath::Min(TargetFog, Fog + FogBuildUpSpeed * DeltaTime)
		: FMath::Max(TargetFog, Fog - FogClearSpeed * DeltaTime);

	if (WipeProgress > 0.f)
	{
		WipeProgress += DeltaTime / WipeTime;
		if (WipeProgress >= 1.f)
		{
			// The hand is past: the visor is clean
			WipeProgress = 0.f;
			Fog = 0.f;
			bWiping = false;
		}
	}

	UpdateMaterial();
}

void UMEVisorComponent::UpdateMaterial()
{
	if (!Material)
	{
		return;
	}

	// Weight 0 skips the pass entirely while the mask is off
	FWeightedBlendables& Blendables = Character->GetFirstPersonCamera()->PostProcessSettings.WeightedBlendables;
	for (FWeightedBlendable& Blendable : Blendables.Array)
	{
		if (Blendable.Object == Material)
		{
			Blendable.Weight = MaskAlpha > 0.f ? 1.f : 0.f;
		}
	}
	if (MaskAlpha <= 0.f)
	{
		return;
	}

	const float Pulse = FMath::Max(Character->GetFirstPersonCamera()->GetBreath(), 0.f) * BreathPulse;
	Material->SetScalarParameterValue(TEXT("MaskAlpha"), MaskAlpha);
	Material->SetScalarParameterValue(TEXT("Fog"), FMath::Clamp(Fog + Fog * Pulse, 0.f, 1.f));
	Material->SetScalarParameterValue(TEXT("WipeProgress"), WipeProgress);
	Material->SetScalarParameterValue(TEXT("Cracks"), static_cast<float>(Cracks));
}
