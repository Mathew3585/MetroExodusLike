#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MEAnimInstance.generated.h"

class AMECharacter;
class UAnimSequenceBase;

/**
 * Computes every locomotion variable the AnimBP needs, and plays the one-shot
 * animations (turn in place, landing) on the default slot.
 * The AnimBP (ABP_MECharacter) only reads these values in its graph, no logic there.
 */
UCLASS()
class METROEXODUSLIKE_API UMEAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Plays the turn-in-place step animation. Returns its duration in seconds (0 if none). */
	float PlayTurnInPlace(bool bTurnRight);

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

	// --- One-shot animations (assigned in ABP_MECharacter class defaults) ---

	/** Slot the one-shot animations play on. Must exist in the AnimGraph. */
	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	FName OneShotSlot = TEXT("DefaultSlot");

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Turn In Place")
	TObjectPtr<UAnimSequenceBase> TurnLeft;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Turn In Place")
	TObjectPtr<UAnimSequenceBase> TurnRight;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Turn In Place")
	TObjectPtr<UAnimSequenceBase> CrouchTurnLeft;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Turn In Place")
	TObjectPtr<UAnimSequenceBase> CrouchTurnRight;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Turn In Place", meta = (ClampMin = "0.1"))
	float TurnPlayRate = 1.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Landing")
	TObjectPtr<UAnimSequenceBase> LandLight;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Landing")
	TObjectPtr<UAnimSequenceBase> LandHeavy;

	/** Landings slower than this play no animation. */
	UPROPERTY(EditDefaultsOnly, Category = "Animations|Landing", meta = (Units = "cm/s"))
	float LightLandingSpeed = 300.f;

	/** Landings faster than this play the heavy animation. */
	UPROPERTY(EditDefaultsOnly, Category = "Animations|Landing", meta = (Units = "cm/s"))
	float HeavyLandingSpeed = 650.f;

	/** How long the landing animation is kept before blending back (the clips have a long recovery). */
	UPROPERTY(EditDefaultsOnly, Category = "Animations|Landing", meta = (Units = "s"))
	float LightLandingDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Animations|Landing", meta = (Units = "s"))
	float HeavyLandingDuration = 0.9f;

	/** Blend out time when movement interrupts a one-shot animation. */
	UPROPERTY(EditDefaultsOnly, Category = "Animations", meta = (Units = "s"))
	float InterruptBlendOut = 0.2f;

private:
	void PlayLanding();

	UPROPERTY(Transient)
	TObjectPtr<AMECharacter> Character;

	bool bWasInAir = false;
	float FallSpeed = 0.f;
	float LandingTimeRemaining = 0.f;
};
