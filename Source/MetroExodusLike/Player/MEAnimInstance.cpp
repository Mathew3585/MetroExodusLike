#include "Player/MEAnimInstance.h"
#include "Player/MECharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UMEAnimInstance::UMEAnimInstance()
{
	// Movement and rotation are driven in C++ (character movement + body rotation).
	// Root motion baked in the clips (landings move ~2 m forward, turns rotate) must never move the character.
	RootMotionMode = ERootMotionMode::NoRootMotionExtraction;
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

	// Landing detection
	if (bIsInAir)
	{
		FallSpeed = FMath::Max(0.f, -Velocity.Z);
	}
	else if (bWasInAir)
	{
		PlayLanding();
	}
	bWasInAir = bIsInAir;

	// One-shots never block movement: moving or jumping blends them out
	if ((bShouldMove || bIsInAir) && IsSlotActive(OneShotSlot))
	{
		StopSlotAnimation(InterruptBlendOut, OneShotSlot);
		LandingTimeRemaining = 0.f;
	}

	if (LandingTimeRemaining > 0.f)
	{
		LandingTimeRemaining -= DeltaSeconds;
		if (LandingTimeRemaining <= 0.f)
		{
			StopSlotAnimation(InterruptBlendOut, OneShotSlot);
		}
	}
}

float UMEAnimInstance::PlayTurnInPlace(bool bTurnRight)
{
	UAnimSequenceBase* Anim = bIsCrouching
		? (bTurnRight ? CrouchTurnRight : CrouchTurnLeft)
		: (bTurnRight ? TurnRight : TurnLeft);

	if (!Anim)
	{
		return 0.f;
	}

	LandingTimeRemaining = 0.f;
	PlaySlotAnimationAsDynamicMontage(Anim, OneShotSlot, 0.15f, 0.3f, TurnPlayRate);
	return Anim->GetPlayLength() / TurnPlayRate;
}

void UMEAnimInstance::PlayLanding()
{
	if (FallSpeed < LightLandingSpeed || bIsCrouching)
	{
		return;
	}

	const bool bHeavy = FallSpeed >= HeavyLandingSpeed;
	UAnimSequenceBase* Anim = bHeavy ? LandHeavy : LandLight;
	if (!Anim)
	{
		return;
	}

	PlaySlotAnimationAsDynamicMontage(Anim, OneShotSlot, 0.05f, 0.3f);
	LandingTimeRemaining = bHeavy ? HeavyLandingDuration : LightLandingDuration;
}
