#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MEAnimInstance.generated.h"

class AMECharacter;

/**
 * Computes every locomotion variable the AnimBP needs.
 * The AnimBP (ABP_MECharacter) only reads these values in its graph, no logic there.
 */
UCLASS()
class METROEXODUSLIKE_API UMEAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** Horizontal speed in cm/s. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed = 0.f;

	/** Movement direction relative to where the body faces, in degrees (-180..180). */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Direction = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsCrouching = false;

	/** Camera pitch in degrees (-90 down .. 90 up), for spine bending later. */
	UPROPERTY(BlueprintReadOnly, Category = "Aim")
	float AimPitch = 0.f;

private:
	UPROPERTY(Transient)
	TObjectPtr<AMECharacter> Character;
};
