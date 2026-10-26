#include "Stealth/MEVisibilityComponent.h"
#include "Player/MECharacter.h"
#include "Survival/MEFlashlightComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/LocalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "UObject/UObjectIterator.h"

UMEVisibilityComponent::UMEVisibilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMEVisibilityComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AMECharacter>(GetOwner());
	UpdateTarget();
	Visibility = TargetVisibility;
}

void UMEVisibilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Character)
	{
		return;
	}

	UpdateTimer -= DeltaTime;
	if (UpdateTimer <= 0.f)
	{
		UpdateTimer = UpdateInterval;
		UpdateTarget();
	}

	// Own lights act at once, the level's light is smoothed
	float Target = TargetVisibility;
	if (Character->GetFlashlight()->IsOn())
	{
		Target = FMath::Max(Target, FlashlightVisibility);
	}
	if (IsFlashing())
	{
		Visibility = FMath::Max(Visibility, ShotFlashVisibility);
		return;
	}
	const float Speed = Target > Visibility ? RiseSpeed : FallSpeed;
	Visibility = FMath::FInterpConstantTo(Visibility, Target, DeltaTime, Speed);
}

void UMEVisibilityComponent::Flash()
{
	FlashUntil = GetWorld()->GetTimeSeconds() + ShotFlashTime;
	Visibility = FMath::Max(Visibility, ShotFlashVisibility);
}

bool UMEVisibilityComponent::IsFlashing() const
{
	return GetWorld()->GetTimeSeconds() < FlashUntil;
}

void UMEVisibilityComponent::UpdateTarget()
{
	if (!Character)
	{
		return;
	}

	// Head, chest, feet: half lit = half seen
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const FVector Center = Capsule->GetComponentLocation();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	TArray<FVector> Points;
	for (const float Height : SampleHeights)
	{
		Points.Add(Center + FVector(0.f, 0.f, HalfHeight * Height));
	}
	if (Points.IsEmpty())
	{
		Points.Add(Center);
	}

	const UWorld* World = GetWorld();
	float Lux = AmbientIlluminance;
	LightCount = 0;
	for (TObjectIterator<ULocalLightComponent> It; It; ++It)
	{
		const ULocalLightComponent* Light = *It;
		if (Light->GetWorld() != World || !Light->IsRegistered() || IsOwnLight(Light->GetOwner()))
		{
			continue;
		}
		const float LightLux = GetLocalLightIlluminance(*Light, Points);
		if (LightLux > 0.f)
		{
			Lux += LightLux;
			++LightCount;
		}
	}
	for (TObjectIterator<UDirectionalLightComponent> It; It; ++It)
	{
		const UDirectionalLightComponent* Sun = *It;
		if (Sun->GetWorld() == World && Sun->IsRegistered())
		{
			const float SunLux = GetSunIlluminance(*Sun, Points);
			if (SunLux > 0.f)
			{
				Lux += SunLux;
				++LightCount;
			}
		}
	}

	// Our eyes are not linear: the first lux count most, then it saturates
	Illuminance = Lux;
	TargetVisibility = 1.f - FMath::Exp(-Lux / HalfVisibleIlluminance);
}

float UMEVisibilityComponent::GetLocalLightIlluminance(const ULocalLightComponent& Light, const TArray<FVector>& Points) const
{
	if (!Light.IsVisible() || !Light.bAffectsWorld || Light.Intensity <= 0.f || Light.AttenuationRadius <= 0.f)
	{
		return 0.f;
	}

	const FVector LightLocation = Light.GetComponentLocation();
	const FVector Target = Points[Points.Num() / 2];
	const float Distance = FVector::Dist(LightLocation, Target);
	if (Distance >= Light.AttenuationRadius)
	{
		return 0.f;
	}

	// Spot: full inside the inner cone, nothing outside the outer one
	float ConeScale = 1.f;
	float CosHalfCone = -1.f;
	const USpotLightComponent* Spot = Cast<USpotLightComponent>(&Light);
	if (Spot)
	{
		const float CosAngle = FVector::DotProduct(Spot->GetDirection(), (Target - LightLocation).GetSafeNormal());
		const float CosOuter = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(Spot->OuterConeAngle, 0.f, 89.f)));
		const float CosInner = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(Spot->InnerConeAngle, 0.f, Spot->OuterConeAngle)));
		ConeScale = FMath::SmoothStep(CosOuter, FMath::Max(CosInner, CosOuter + KINDA_SMALL_NUMBER), CosAngle);
		CosHalfCone = Spot->GetCosHalfConeAngle();
		if (ConeScale <= 0.f)
		{
			return 0.f;
		}
	}

	// Same falloff as the renderer: inverse square with a window at the radius, or the old exponent falloff
	float Lux = 0.f;
	const float Ratio = Distance / Light.AttenuationRadius;
	const UPointLightComponent* Point = Cast<UPointLightComponent>(&Light);
	if (!Point || Point->bUseInverseSquaredFalloff)
	{
		const ELightUnits Units = Light.IntensityUnits == ELightUnits::EV ? ELightUnits::Candelas : Light.IntensityUnits;
		const float Intensity = Light.IntensityUnits == ELightUnits::EV ? FMath::Pow(2.f, Light.Intensity - 3.f) : Light.Intensity;
		const float Candelas = Intensity * ULocalLightComponent::GetUnitsConversionFactor(Units, ELightUnits::Candelas, CosHalfCone);
		const float Meters = FMath::Max(Distance / 100.f, 0.1f);
		const float Window = FMath::Square(FMath::Clamp(1.f - FMath::Pow(Ratio, 4.f), 0.f, 1.f));
		Lux = Candelas / FMath::Square(Meters) * Window;
	}
	else
	{
		Lux = Light.Intensity * UnitlessLightScale * FMath::Pow(1.f - Ratio, Point->LightFalloffExponent);
	}

	// Dim coloured lights (red emergency bulbs) show less than white ones
	Lux *= FLinearColor(Light.LightColor).GetLuminance() * ConeScale;
	if (Lux <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	// A light that casts no shadow lights through walls on screen too
	return Light.CastShadows ? Lux * GetReachedShare(LightLocation, Points, Light.GetOwner()) : Lux;
}

float UMEVisibilityComponent::GetSunIlluminance(const UDirectionalLightComponent& Sun, const TArray<FVector>& Points) const
{
	if (!Sun.IsVisible() || !Sun.bAffectsWorld || Sun.Intensity <= 0.f)
	{
		return 0.f;
	}

	// Directional intensity is already in lux; anything above along the rays (a ceiling) hides it
	const float Lux = Sun.Intensity * FLinearColor(Sun.LightColor).GetLuminance();
	if (!Sun.CastShadows)
	{
		return Lux;
	}

	const FVector ToSun = -Sun.GetDirection();
	int32 Reached = 0;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MEVisibilitySun), false, Character);
	TArray<AActor*> Attached;
	Character->GetAttachedActors(Attached, true, true);
	Params.AddIgnoredActors(Attached);
	for (const FVector& Point : Points)
	{
		if (!GetWorld()->LineTraceTestByChannel(Point, Point + ToSun * SunTraceDistance, ECC_Visibility, Params))
		{
			++Reached;
		}
	}
	return Lux * Reached / Points.Num();
}

float UMEVisibilityComponent::GetReachedShare(const FVector& From, const TArray<FVector>& Points, const AActor* LightOwner) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MEVisibility), false, Character);
	TArray<AActor*> Attached;
	Character->GetAttachedActors(Attached, true, true);
	Params.AddIgnoredActors(Attached);
	// The lamp's own mesh around the bulb does not shadow it
	Params.AddIgnoredActor(LightOwner);

	int32 Reached = 0;
	for (const FVector& Point : Points)
	{
		if (!GetWorld()->LineTraceTestByChannel(From, Point, ECC_Visibility, Params))
		{
			++Reached;
		}
	}
	return static_cast<float>(Reached) / Points.Num();
}

bool UMEVisibilityComponent::IsOwnLight(const AActor* LightOwner) const
{
	// The flashlight is on the character, the muzzle light on a weapon attached to it
	return LightOwner && (LightOwner == Character || LightOwner->IsAttachedTo(Character) || LightOwner->GetOwner() == Character);
}
