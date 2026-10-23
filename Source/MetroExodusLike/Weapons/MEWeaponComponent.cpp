#include "Weapons/MEWeaponComponent.h"
#include "Weapons/MEWeapon.h"
#include "Weapons/MEWeaponData.h"
#include "Player/MECharacter.h"
#include "Player/MEFirstPersonAnimInstance.h"
#include "Player/MEFirstPersonCameraComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

UMEWeaponComponent::UMEWeaponComponent()
{
	// Ticks for the wall check
	PrimaryComponentTick.bCanEverTick = true;
	WeaponClass = AMEWeapon::StaticClass();
}

UMEWeaponData* UMEWeaponComponent::GetWeaponData() const
{
	return Weapon ? Weapon->GetData() : nullptr;
}

void UMEWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateWallBlock(DeltaTime);
}

void UMEWeaponComponent::UpdateWallBlock(float DeltaTime)
{
	const UMEWeaponData* Data = GetWeaponData();
	if (!Data || !Character)
	{
		return;
	}

	// Free space in front of the eye, along the view
	const UMEFirstPersonCameraComponent* Camera = Character->GetFirstPersonCamera();
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * Data->WallCheckLength;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MEWeaponWallCheck), false, Character);
	Params.AddIgnoredActor(Weapon);
	Params.AddIgnoredActor(WorldWeapon);

	FHitResult Hit;
	const bool bHit = GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(WallCheckRadius), Params);

	// 0 at the check length, 1 at the against-wall distance
	const float Distance = bHit ? Hit.Distance : Data->WallCheckLength;
	const float Target = 1.f - FMath::GetRangePct(Data->AgainstWallDistance, Data->WallCheckLength, FMath::Clamp(Distance, Data->AgainstWallDistance, Data->WallCheckLength));
	WallBlockAmount = FMath::FInterpTo(WallBlockAmount, Target, DeltaTime, WallBlockBlendSpeed);
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
	Weapon->AttachToComponent(Arms, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponData->ArmsGripBone);
	Weapon->SetActorRelativeTransform(WeaponData->ArmsGripOffset);

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
