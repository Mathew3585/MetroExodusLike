#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MEFootstepComponent.generated.h"

class ACharacter;
class UMEFootstepData;

/**
 * Footstep sounds read from the body's feet: a step is a foot coming back down after being lifted,
 * so the sounds land exactly on the steps seen on screen, whatever the animation (no notifies needed).
 * The surface under the foot picks the sound. Also plays the landing of jumps and falls.
 */
UCLASS(ClassGroup = (Player), meta = (BlueprintSpawnableComponent))
class METROEXODUSLIKE_API UMEFootstepComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMEFootstepComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Both feet hit the ground. Strength: 0 = soft, 1 = heavy landing (louder above). */
	void PlayLanding(float Strength);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Footsteps")
	TObjectPtr<UMEFootstepData> FootstepData;

	/** Foot bones of the body mesh (ankles). */
	UPROPERTY(EditDefaultsOnly, Category = "Footsteps")
	TArray<FName> FootBones = { TEXT("foot_l"), TEXT("foot_r") };

	/** The foot counts as lifted above this height (ankle above the capsule bottom; at rest it is about 9 cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Footsteps", meta = (ClampMin = "0", Units = "cm"))
	float LiftHeight = 15.f;

	/** A lifted foot coming back under this height is a step. */
	UPROPERTY(EditDefaultsOnly, Category = "Footsteps", meta = (ClampMin = "0", Units = "cm"))
	float PlantHeight = 11.f;

	/** Two steps of the same foot closer than this are one (animation blends can wobble around the thresholds). */
	UPROPERTY(EditDefaultsOnly, Category = "Footsteps", meta = (ClampMin = "0", Units = "s"))
	float MinStepInterval = 0.2f;

private:
	void PlayStep(const FVector& FootLocation, float Volume);

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Character;

	struct FFootState
	{
		bool bLifted = false;
		double LastStepTime = -1.0;
	};
	TArray<FFootState> Feet;
};
