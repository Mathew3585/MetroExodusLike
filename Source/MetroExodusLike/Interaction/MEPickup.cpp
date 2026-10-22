#include "Interaction/MEPickup.h"
#include "MetroExodusLike.h"
#include "Components/StaticMeshComponent.h"

#define LOCTEXT_NAMESPACE "MEPickup"

AMEPickup::AMEPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
}

FMEInteractionInfo AMEPickup::GetInteractionInfo(const AMECharacter* Interactor) const
{
	FMEInteractionInfo Info;
	Info.Gesture = EMEInteractionGesture::Pickup;
	Info.ActionText = Quantity > 1
		? FText::Format(LOCTEXT("PickUpQuantity", "Pick up {0} ({1})"), ItemName, Quantity)
		: FText::Format(LOCTEXT("PickUp", "Pick up {0}"), ItemName);
	return Info;
}

void AMEPickup::Interact(AMECharacter* Interactor)
{
	UE_LOG(LogME, Log, TEXT("Picked up %d x %s"), Quantity, *ItemName.ToString());
	Destroy();
}

#undef LOCTEXT_NAMESPACE
