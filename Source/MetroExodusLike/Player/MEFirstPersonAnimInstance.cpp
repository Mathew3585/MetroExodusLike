#include "Player/MEFirstPersonAnimInstance.h"
#include "Player/MECharacter.h"
#include "Weapons/MEWeaponData.h"
#include "Weapons/MEWeaponComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimMontage.h"

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

	const UMEWeaponComponent* Weapons = Character->GetWeapons();
	AimAlpha = AimLoop ? Weapons->GetAimAlpha() : 0.f;

	// Kick and sway are in view space; the arms mesh is turned relative to the view pivot
	const FQuat ArmsRotation = GetSkelMeshComponent()->GetRelativeRotation().Quaternion();
	const FTransform& Sway = Weapons->GetArmsSway();
	ArmsOffset = FTransform(
		ArmsRotation.Inverse() * Sway.GetRotation() * ArmsRotation,
		ArmsRotation.UnrotateVector(Sway.GetTranslation() + Weapons->GetArmsKick()));

	// First half of the wall block pulls the weapon back, second half raises it
	const float WallBlock = Weapons->GetWallBlockAmount();
	CloseToWallAlpha = CloseToWallPose ? FMath::Clamp(WallBlock * 2.f, 0.f, 1.f) : 0.f;
	AgainstWallAlpha = AgainstWallPose ? FMath::Clamp(WallBlock * 2.f - 1.f, 0.f, 1.f) : 0.f;
}

void UMEFirstPersonAnimInstance::SetWeaponData(const UMEWeaponData* WeaponData)
{
	IdleLoop = WeaponData ? WeaponData->IdleLoop : nullptr;
	WalkLoop = WeaponData ? WeaponData->WalkLoop : nullptr;
	SprintLoop = WeaponData ? WeaponData->SprintLoop : nullptr;
	AimLoop = WeaponData ? WeaponData->AimLoop : nullptr;
	AimWalkLoop = WeaponData && WeaponData->AimWalkLoop ? WeaponData->AimWalkLoop : AimLoop;
	CloseToWallPose = WeaponData ? WeaponData->CloseToWallPose : nullptr;
	AgainstWallPose = WeaponData ? WeaponData->AgainstWallPose : nullptr;
}

void UMEFirstPersonAnimInstance::StopAction(float BlendOutTime)
{
	StopSlotAnimation(BlendOutTime, ActionSlot);
}

void UMEFirstPersonAnimInstance::PlayAction(UAnimSequenceBase* Animation, bool bHoldLastFrame)
{
	if (!Animation)
	{
		return;
	}

	// The dynamic montage is made for this play only: changing it affects nothing else
	if (UAnimMontage* Montage = PlaySlotAnimationAsDynamicMontage(Animation, ActionSlot, 0.1f, 0.15f))
	{
		Montage->bEnableAutoBlendOut = !bHoldLastFrame;
	}
}
