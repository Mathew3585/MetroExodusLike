#include "Weapons/MEWeaponComponent.h"
#include "Weapons/MEWeapon.h"
#include "Weapons/MEWeaponData.h"
#include "Player/MECharacter.h"
#include "Player/MEFirstPersonAnimInstance.h"
#include "Engine/World.h"

UMEWeaponComponent::UMEWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	WeaponClass = AMEWeapon::StaticClass();
}

UMEWeaponData* UMEWeaponComponent::GetWeaponData() const
{
	return Weapon ? Weapon->GetData() : nullptr;
}

void UMEWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AMECharacter>(GetOwner());
	if (Character && StartingWeapon)
	{
		Equip(StartingWeapon);
	}
}

void UMEWeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (AMEWeapon* Spawned : { Weapon.Get(), WorldWeapon.Get() })
	{
		if (Spawned)
		{
			Spawned->Destroy();
		}
	}
	Weapon = nullptr;
	WorldWeapon = nullptr;

	Super::EndPlay(EndPlayReason);
}

void UMEWeaponComponent::Equip(UMEWeaponData* WeaponData)
{
	FActorSpawnParameters Params;
	Params.Owner = Character;
	Params.Instigator = Character;

	// First-person weapon, in the arms
	Weapon = GetWorld()->SpawnActor<AMEWeapon>(WeaponClass, Params);
	Weapon->Initialize(WeaponData, true);

	USkeletalMeshComponent* Arms = Character->GetFirstPersonArms();
	Weapon->AttachToComponent(Arms, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponData->GripBone);
	Weapon->SetActorRelativeTransform(WeaponData->GripOffset);

	if (UMEFirstPersonAnimInstance* ArmsAnim = Cast<UMEFirstPersonAnimInstance>(Arms->GetAnimInstance()))
	{
		ArmsAnim->SetWeaponData(WeaponData);
	}

	// World weapon, in the world body's hands: casts the shadow, seen by the AI
	WorldWeapon = GetWorld()->SpawnActor<AMEWeapon>(WeaponClass, Params);
	WorldWeapon->Initialize(WeaponData, false);
	WorldWeapon->AttachToComponent(Character->GetWorldBody(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponData->GripBone);
	WorldWeapon->SetActorRelativeTransform(WeaponData->GripOffset);

	Character->SetBodyWeaponPose(WeaponData->BodyHoldPose);

	PlayAction(WeaponData->Draw);
}

void UMEWeaponComponent::PlayAction(const FMEWeaponAnimation& Action)
{
	if (UMEFirstPersonAnimInstance* ArmsAnim = Cast<UMEFirstPersonAnimInstance>(Character->GetFirstPersonArms()->GetAnimInstance()))
	{
		ArmsAnim->PlayAction(Action.Arms);
	}
	Weapon->PlayWeaponAnimation(Action.Weapon);
}
