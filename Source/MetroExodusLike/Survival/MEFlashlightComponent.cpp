#include "Survival/MEFlashlightComponent.h"
#include "Kismet/GameplayStatics.h"

UMEFlashlightComponent::UMEFlashlightComponent()
{
	// A narrow, strong beam that reaches down a tunnel, with a soft edge
	SetIntensityUnits(ELightUnits::Lumens);
	Intensity = 1200.f;
	AttenuationRadius = 3000.f;
	InnerConeAngle = 14.f;
	OuterConeAngle = 30.f;
	SourceRadius = 2.f;
	LightColor = FColor(255, 238, 214); // Slightly warm, an old incandescent bulb
	SetCastShadows(true);
	SetVisibility(false);

	// Only ticks while flickering
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UMEFlashlightComponent::BeginPlay()
{
	Super::BeginPlay();

	BaseIntensity = Intensity;
	bIsOn = bStartsOn;
	SetVisibility(bIsOn);
}

void UMEFlashlightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FlickerRemaining -= DeltaTime;
	if (FlickerRemaining <= 0.f)
	{
		StopFlicker();
		return;
	}

	NextFlickerJump -= DeltaTime;
	if (NextFlickerJump > 0.f)
	{
		return;
	}
	NextFlickerJump = 1.f / FlickerRate;

	// Blackouts get rarer and the light steadier as the bulb settles
	const float Progress = 1.f - FlickerRemaining / FlickerTime;
	const bool bBlackout = FMath::FRand() < FlickerOffChance * (1.f - Progress);
	const float Scale = bBlackout ? 0.f : FMath::FRandRange(FMath::Lerp(0.2f, 0.8f, Progress), 1.f);
	SetIntensity(BaseIntensity * Scale);
}

void UMEFlashlightComponent::StopFlicker()
{
	FlickerRemaining = 0.f;
	SetIntensity(BaseIntensity);
	SetComponentTickEnabled(false);
}

void UMEFlashlightComponent::Toggle()
{
	SetOn(!bIsOn);
}

void UMEFlashlightComponent::SetOn(bool bOn)
{
	if (bOn == bIsOn)
	{
		return;
	}

	bIsOn = bOn;
	SetVisibility(bIsOn);
	if (bIsOn && FlickerTime > 0.f)
	{
		FlickerRemaining = FlickerTime;
		NextFlickerJump = 0.f;
		SetComponentTickEnabled(true);
	}
	else
	{
		StopFlicker();
	}
	if (USoundBase* Sound = bIsOn ? SwitchOnSound : SwitchOffSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetComponentLocation());
	}
}
