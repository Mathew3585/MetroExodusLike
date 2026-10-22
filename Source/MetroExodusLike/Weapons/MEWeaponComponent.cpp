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
	if (Weapon)
	{
		Weapon->Destroy();
		Weapon = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UMEWeaponComponent::Equip(UMEWeaponData* WeaponData)
{
	FActorSpawnParameters Params;
	Params.Owner = Character;
	Params.Instigator = Character;
	Weapon = GetWorld()->SpawnActor<AMEWeapon>(WeaponClass, Params);
	Weapon->Initialize(WeaponData);

	USkeletalMeshComponent* Arms = Character->GetFirstPersonArms();
	Weapon->AttachToComponent(Arms, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponData->ArmsAttachBone);

	if (UMEFirstPersonAnimInstance* ArmsAnim = Cast<UMEFirstPersonAnimInstance>(Arms->GetAnimInstance()))
	{
		ArmsAnim->SetWeaponData(WeaponData);
	}

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
