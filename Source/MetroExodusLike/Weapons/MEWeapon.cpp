#include "Weapons/MEWeapon.h"
#include "Weapons/MEWeaponData.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

AMEWeapon::AMEWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMEWeapon::Initialize(UMEWeaponData* InData, bool bFirstPerson)
{
	Data = InData;
	if (!Data)
	{
		return;
	}

	Mesh->SetSkeletalMeshAsset(Data->Mesh);
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	TArray<UPrimitiveComponent*> Visuals = { Mesh };
	for (const FMEWeaponPart& Part : Data->Parts)
	{
		UStaticMeshComponent* PartComponent = NewObject<UStaticMeshComponent>(this);
		PartComponent->SetStaticMesh(Part.Mesh);
		PartComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PartComponent->SetupAttachment(Mesh, Part.Bone);
		PartComponent->RegisterComponent();
		Visuals.Add(PartComponent);
	}

	for (UPrimitiveComponent* Visual : Visuals)
	{
		if (bFirstPerson)
		{
			// Drawn with the view model FOV, the world weapon casts the shadow
			Visual->SetCastShadow(false);
			Visual->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
		}
		else
		{
			Visual->SetHiddenInGame(true);
			Visual->bCastHiddenShadow = true;
		}
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
