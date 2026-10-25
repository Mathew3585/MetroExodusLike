#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MEFootstepData.generated.h"

class UPhysicalMaterial;
class USoundBase;

/**
 * Footstep sounds per surface: the physical material under the foot picks the sound (PM_Concrete, PM_Metal...).
 * Sounds are random cues (several takes) so steps never repeat. One asset per kind of feet (DA_Footsteps_Boots...).
 */
UCLASS(BlueprintType)
class METROEXODUSLIKE_API UMEFootstepData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	USoundBase* FindSound(const UPhysicalMaterial* Surface) const;

	/** Surfaces without a physical material, or not listed below. */
	UPROPERTY(EditDefaultsOnly, Category = "Footsteps")
	TObjectPtr<USoundBase> Default;

	UPROPERTY(EditDefaultsOnly, Category = "Footsteps")
	TMap<TObjectPtr<UPhysicalMaterial>, TObjectPtr<USoundBase>> Surfaces;

	// How loud a step is, per gait
	UPROPERTY(EditDefaultsOnly, Category = "Volume", meta = (ClampMin = "0"))
	float WalkVolume = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Volume", meta = (ClampMin = "0"))
	float SprintVolume = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Volume", meta = (ClampMin = "0"))
	float CrouchVolume = 0.2f;

	/** Landing from a jump or a fall at the heavy landing speed (lighter landings are quieter). */
	UPROPERTY(EditDefaultsOnly, Category = "Volume", meta = (ClampMin = "0"))
	float LandingVolume = 1.2f;

	// How far enemies hear a step, per gait (share of their hearing range: 0.25 = a quarter of it)
	UPROPERTY(EditDefaultsOnly, Category = "Noise", meta = (ClampMin = "0"))
	float WalkNoise = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Noise", meta = (ClampMin = "0"))
	float SprintNoise = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Noise", meta = (ClampMin = "0"))
	float CrouchNoise = 0.05f;

	/** Heavy landing (lighter landings are quieter). */
	UPROPERTY(EditDefaultsOnly, Category = "Noise", meta = (ClampMin = "0"))
	float LandingNoise = 0.6f;

	/** Random pitch, either way (0.05 = +/- 5%). */
	UPROPERTY(EditDefaultsOnly, Category = "Volume", meta = (ClampMin = "0", ClampMax = "0.5"))
	float PitchVariation = 0.05f;
};
