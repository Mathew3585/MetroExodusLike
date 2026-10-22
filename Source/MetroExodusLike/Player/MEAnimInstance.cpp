#include "Player/MEAnimInstance.h"
#include "Player/MECharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UMEAnimInstance::UMEAnimInstance()
{
	// Movement and rotation are driven in C++ (character movement + body rotation).
	// Root motion baked in the clips (landings move ~2 m forward, turns rotate) must never move the character.
	RootMotionMode = ERootMotionMode::NoRootMotionExtraction;

	LandLight.ContactTime = 0.48f;
	LandLight.HoldTime = 0.7f;
	LandLightMoving.ContactTime = 0.48f;
	LandLightMoving.HoldTime = 0.5f;
	LandHeavy.ContactTime = 0.95f;
	LandHeavy.HoldTime = 1.2f;
}

void UMEAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<AMECharacter>(TryGetPawnOwner());
}

void UMEAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Character)
	{
		return;
	}

	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const FVector Velocity = Movement->Velocity;

	GroundSpeed = Velocity.Size2D();
	bShouldMove = GroundSpeed > 3.f && !Movement->GetCurrentAcceleration().IsNearlyZero();
	bIsInAir = Movement->IsFalling();
	bIsSprinting = Character->IsSprinting();
	bIsCrouching = Movement->IsCrouching();

	Direction = bShouldMove
		? FRotator::NormalizeAxis(Velocity.Rotation().Yaw - Character->GetActorRotation().Yaw)
		: 0.f;

	AimPitch = FRotator::NormalizeAxis(Character->GetBaseAimRotation().Pitch);

	if (!IsSlotActive(OneShotSlot))
	{
		return;
	}

	// Jumping always cuts a one-shot, moving only cuts the interruptible ones (turns, standing landings)
	if (bIsInAir || (bShouldMove && bOneShotInterruptible))
	{
		StopSlotAnimation(InterruptBlendOut, OneShotSlot);
		OneShotTimeRemaining = 0.f;
		return;
	}

	if (OneShotTimeRemaining > 0.f)
	{
		OneShotTimeRemaining -= DeltaSeconds;
		if (OneShotTimeRemaining <= 0.f)
		{
			StopSlotAnimation(InterruptBlendOut, OneShotSlot);
		}
	}
}

void UMEAnimInstance::PlayTurnInPlace(bool bTurnRight)
{
	UAnimSequenceBase* Anim = bIsCrouching
		? (bTurnRight ? CrouchTurnRight : CrouchTurnLeft)
		: (bTurnRight ? TurnRight : TurnLeft);

	PlayOneShot(Anim, TurnPlayRate, 0.f, 0.f, true);
}

void UMEAnimInstance::PlayLanding(bool bHeavy)
{
	if (bHeavy)
	{
		PlayOneShot(LandHeavy.Animation, 1.f, LandHeavy.ContactTime, LandHeavy.HoldTime, false);
		return;
	}

	// Crouched light landings (walking off a ledge) need no animation
	if (bIsCrouching)
	{
		return;
	}

	const bool bMovingForward = bShouldMove && FMath::Abs(Direction) <= MovingLandingMaxAngle;
	const FMELandingAnimation& Landing = bMovingForward ? LandLightMoving : LandLight;
	PlayOneShot(Landing.Animation, 1.f, Landing.ContactTime, Landing.HoldTime, !bMovingForward);
}

void UMEAnimInstance::PlayOneShot(UAnimSequenceBase* Animation, float PlayRate, float StartTime, float HoldTime, bool bInterruptible)
{
	if (!Animation)
	{
		return;
	}

	UAnimMontage* Montage = PlaySlotAnimationAsDynamicMontage(Animation, OneShotSlot, 0.1f, 0.3f, PlayRate);
	if (Montage && StartTime > 0.f)
	{
		Montage_SetPosition(Montage, StartTime);
	}

	OneShotTimeRemaining = HoldTime;
	bOneShotInterruptible = bInterruptible;
}
