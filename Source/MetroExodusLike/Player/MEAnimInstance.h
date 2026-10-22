#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MEAnimInstance.generated.h"

class AMECharacter;
class UAnimSequenceBase;

/** A landing clip. The GASP clips start in the air, so we skip to the foot contact. */
USTRUCT(BlueprintType)
struct FMELandingAnimation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimSequenceBase> Animation;

	/** Time of foot contact in the clip. */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "s"))
	float ContactTime = 0.f;

	/** How long the clip is kept after the contact before blending back (the clips have a long recovery). */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", Units = "s"))
	float HoldTime = 0.7f;
};

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
	UMEAnimInstance();

	/** Plays the turn-in-place step animation. */
	void PlayTurnInPlace(bool bTurnRight);

	/** Plays the landing clip. Heavy always plays the stop-and-absorb clip, light depends on movement. */
	void PlayLanding(bool bHeavy);

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

	/** Blend out time when movement interrupts a one-shot animation. */
	UPROPERTY(EditDefaultsOnly, Category = "Animations", meta = (Units = "s"))
	float InterruptBlendOut = 0.2f;

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

	/** Light landing while standing still. Interrupted by movement. */
	UPROPERTY(EditDefaultsOnly, Category = "Animations|Landing")
	FMELandingAnimation LandLight;

	/** Light landing while moving forward. Plays through the movement. */
	UPROPERTY(EditDefaultsOnly, Category = "Animations|Landing")
	FMELandingAnimation LandLightMoving;

	/** Heavy landing: the character stops, absorbs and looks down. Never interrupted. */
	UPROPERTY(EditDefaultsOnly, Category = "Animations|Landing")
	FMELandingAnimation LandHeavy;

	/** Light landings use the moving clip only when moving within this angle of the body's forward. */
	UPROPERTY(EditDefaultsOnly, Category = "Animations|Landing", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float MovingLandingMaxAngle = 60.f;

private:
	void PlayOneShot(UAnimSequenceBase* Animation, float PlayRate, float StartTime, float HoldTime, bool bInterruptible);

	UPROPERTY(Transient)
	TObjectPtr<AMECharacter> Character;

	/** Remaining time before the current one-shot blends out (0 = until it ends). */
	float OneShotTimeRemaining = 0.f;

	/** Whether movement may cut the current one-shot. */
	bool bOneShotInterruptible = true;
};
