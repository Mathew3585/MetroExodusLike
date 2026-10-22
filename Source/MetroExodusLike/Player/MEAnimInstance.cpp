#include "Player/MEAnimInstance.h"
#include "Player/MECharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

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

	Direction = bShouldMove
		? FRotator::NormalizeAxis(Velocity.Rotation().Yaw - Character->GetActorRotation().Yaw)
		: 0.f;

	AimPitch = FRotator::NormalizeAxis(Character->GetBaseAimRotation().Pitch);
}
