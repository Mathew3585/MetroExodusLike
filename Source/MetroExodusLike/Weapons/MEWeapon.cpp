#include "Weapons/MEWeapon.h"
#include "Weapons/MEWeaponData.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

AMEWeapon::AMEWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// Held in first person: drawn with the view model FOV, no collision, no world shadow
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
}

void AMEWeapon::Initialize(UMEWeaponData* InData)
{
	Data = InData;
	if (!Data)
	{
		return;
	}

	Mesh->SetSkeletalMeshAsset(Data->Mesh);
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	for (const FMEWeaponPart& Part : Data->Parts)
	{
		UStaticMeshComponent* PartComponent = NewObject<UStaticMeshComponent>(this);
		PartComponent->SetStaticMesh(Part.Mesh);
		PartComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PartComponent->SetCastShadow(false);
		PartComponent->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
		PartComponent->SetupAttachment(Mesh, Part.Bone);
		PartComponent->RegisterComponent();
	}

	AmmoInMagazine = Data->MagazineSize;
	ReserveAmmo = Data->StartingReserveAmmo;
}

void AMEWeapon::PlayWeaponAnimation(UAnimSequenceBase* Animation)
{
	if (Animation)
	{
		Mesh->PlayAnimation(Animation, false);
	}
}
