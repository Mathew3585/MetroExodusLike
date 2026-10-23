#include "Interaction/MEPickup.h"
#include "MetroExodusLike.h"
#include "Player/MECharacter.h"
#include "Inventory/MEInventoryComponent.h"
#include "Inventory/MEItemData.h"
#include "Components/StaticMeshComponent.h"

#define LOCTEXT_NAMESPACE "MEPickup"

AMEPickup::AMEPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
}

void AMEPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (Item && Item->WorldMesh && !Mesh->GetStaticMesh())
	{
		Mesh->SetStaticMesh(Item->WorldMesh);
	}
}

FMEInteractionInfo AMEPickup::GetInteractionInfo(const AMECharacter* Interactor) const
{
	FMEInteractionInfo Info;
	Info.Gesture = EMEInteractionGesture::Pickup;

	if (!Item)
	{
		Info.bCanInteract = false;
		return Info;
	}

	// Full: shown but not usable, so the player knows why
	if (Interactor && Interactor->GetInventory()->GetRoomFor(Item) == 0)
	{
		Info.bCanInteract = false;
		Info.ActionText = FText::Format(LOCTEXT("Full", "{0} full"), Item->DisplayName);
		return Info;
	}

	Info.ActionText = Quantity > 1
		? FText::Format(LOCTEXT("PickUpQuantity", "Pick up {0} ({1})"), Item->DisplayName, Quantity)
		: FText::Format(LOCTEXT("PickUp", "Pick up {0}"), Item->DisplayName);
	return Info;
}

void AMEPickup::Interact(AMECharacter* Interactor)
{
	if (!Item || !Interactor)
	{
		return;
	}

	const int32 Added = Interactor->GetInventory()->Add(Item, Quantity);
	UE_LOG(LogME, Log, TEXT("Picked up %d x %s"), Added, *Item->DisplayName.ToString());

	Quantity -= Added;
	if (Quantity <= 0)
	{
		Destroy();
	}
}

#undef LOCTEXT_NAMESPACE
