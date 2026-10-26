#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MEVisibilityComponent.generated.h"

class AMECharacter;
class ULocalLightComponent;
class UDirectionalLightComponent;

/**
 * How visible the player is, from the light falling on him: 0 = pitch dark, 1 = fully lit.
 * Read by the enemies' sight (range and detection speed).
 *
 * Computed from the level's lights, not from the rendered image (cheap, steady, works with Lumen):
 * each light in range adds its light at the player (intensity, falloff, spot cone, colour),
 * scaled by how many of the head / chest / feet points it reaches (shadows), plus the sun and an ambient level.
 * The player's own lights are special: the flashlight lights what is in front, not him (a visibility floor while on),
 * and each shot flashes him for a moment.
 */
UCLASS(ClassGroup = (Stealth), meta = (BlueprintSpawnableComponent))
class METROEXODUSLIKE_API UMEVisibilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMEVisibilityComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 0 = pitch dark, 1 = fully lit. Smoothed: rises fast, falls slower. */
	float GetVisibility() const { return Visibility; }

	/** Light at the player from the level (lux), before the flashlight and shots. */
	float GetIlluminance() const { return Illuminance; }
	int32 GetLightCount() const { return LightCount; }

	/** A shot: the muzzle flash shows the player for a moment. */
	void Flash();
	bool IsFlashing() const;

protected:
	virtual void BeginPlay() override;

	/** Light everywhere, whatever the lights (sky, bounce). Raise it on bright maps. */
	UPROPERTY(EditDefaultsOnly, Category = "Visibility", meta = (ClampMin = "0", Units = "Lux"))
	float AmbientIlluminance = 0.2f;

	/** Light at which the player is about two thirds visible (1 - 1/e). Lower = the dark hides less. */
	UPROPERTY(EditDefaultsOnly, Category = "Visibility", meta = (ClampMin = "0.01", Units = "Lux"))
	float HalfVisibleIlluminance = 8.f;

	/** Lights with no physical falloff (unitless intensity): their intensity counts as this much light at the source. */
	UPROPERTY(EditDefaultsOnly, Category = "Visibility", meta = (ClampMin = "0", Units = "Lux"))
	float UnitlessLightScale = 10.f;

	/** The light is checked this often. */
	UPROPERTY(EditDefaultsOnly, Category = "Visibility", meta = (ClampMin = "0.02", Units = "s"))
	float UpdateInterval = 0.1f;

	/** Visibility change per second: stepping into light shows at once, stepping into shadow takes a moment. */
	UPROPERTY(EditDefaultsOnly, Category = "Visibility", meta = (ClampMin = "0"))
	float RiseSpeed = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "Visibility", meta = (ClampMin = "0"))
	float FallSpeed = 2.f;

	/** Points of the body the light must reach, as heights in the capsule (-1 = feet, 1 = top of the head). */
	UPROPERTY(EditDefaultsOnly, Category = "Visibility")
	TArray<float> SampleHeights = { 0.85f, 0.2f, -0.8f };

	/** Visibility while the flashlight is on, however dark it is around. */
	UPROPERTY(EditDefaultsOnly, Category = "Visibility|Own Lights", meta = (ClampMin = "0", ClampMax = "1"))
	float FlashlightVisibility = 0.7f;

	/** A shot shows the player this much, this long. */
	UPROPERTY(EditDefaultsOnly, Category = "Visibility|Own Lights", meta = (ClampMin = "0", ClampMax = "1"))
	float ShotFlashVisibility = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Visibility|Own Lights", meta = (ClampMin = "0", Units = "s"))
	float ShotFlashTime = 0.15f;

	/** The sun is blocked by anything this far up along its rays (roofs, tunnel ceilings). */
	UPROPERTY(EditDefaultsOnly, Category = "Visibility", meta = (ClampMin = "0", Units = "cm"))
	float SunTraceDistance = 20000.f;

private:
	void UpdateTarget();
	float GetLocalLightIlluminance(const ULocalLightComponent& Light, const TArray<FVector>& Points) const;
	float GetSunIlluminance(const UDirectionalLightComponent& Sun, const TArray<FVector>& Points) const;

	/** Share of the points reached from this location (0..1). */
	float GetReachedShare(const FVector& From, const TArray<FVector>& Points, const AActor* LightOwner) const;

	/** The player and what he carries: their lights are handled apart, and they never shadow him. */
	bool IsOwnLight(const AActor* LightOwner) const;

	UPROPERTY()
	TObjectPtr<AMECharacter> Character;

	float Visibility = 0.f;
	float TargetVisibility = 0.f;
	float Illuminance = 0.f;
	int32 LightCount = 0;
	float UpdateTimer = 0.f;
	double FlashUntil = -1.0;
};
