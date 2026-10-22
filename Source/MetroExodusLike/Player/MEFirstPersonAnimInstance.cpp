#include "Player/MEFirstPersonAnimInstance.h"
#include "Player/MECharacter.h"
#include "Weapons/MEWeaponData.h"
#include "GameFramework/CharacterMovementComponent.h"

UMEFirstPersonAnimInstance::UMEFirstPersonAnimInstance()
{
	// The arms are attached to the camera: their clips must never move them
	RootMotionMode = ERootMotionMode::NoRootMotionExtraction;
}

void UMEFirstPersonAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<AMECharacter>(TryGetPawnOwner());
}

void UMEFirstPersonAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Character)
	{
		return;
	}

	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	bShouldMove = Movement->IsMovingOnGround()
		&& Movement->Velocity.Size2D() > 10.f
		&& !Movement->GetCurrentAcceleration().IsNearlyZero();
	bIsSprinting = Character->IsSprinting();
}

void UMEFirstPersonAnimInstance::SetWeaponData(const UMEWeaponData* WeaponData)
{
	IdleLoop = WeaponData ? WeaponData->IdleLoop : nullptr;
	WalkLoop = WeaponData ? WeaponData->WalkLoop : nullptr;
	SprintLoop = WeaponData ? WeaponData->SprintLoop : nullptr;
}

void UMEFirstPersonAnimInstance::PlayAction(UAnimSequenceBase* Animation)
{
	if (Animation)
	{
		PlaySlotAnimationAsDynamicMontage(Animation, ActionSlot, 0.1f, 0.15f);
	}
}
