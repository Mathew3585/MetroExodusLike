#include "Inventory/MEInventoryComponent.h"
#include "Inventory/MEItemData.h"

UMEInventoryComponent::UMEInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMEInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	for (const TPair<TObjectPtr<UMEItemData>, int32>& Starting : StartingItems)
	{
		Add(Starting.Key, Starting.Value);
	}
}

int32 UMEInventoryComponent::GetCount(const UMEItemData* Item) const
{
	const int32* Count = Items.Find(Item);
	return Count ? *Count : 0;
}

int32 UMEInventoryComponent::GetRoomFor(const UMEItemData* Item) const
{
	return Item ? FMath::Max(0, Item->MaxCarry - GetCount(Item)) : 0;
}

int32 UMEInventoryComponent::Add(UMEItemData* Item, int32 Quantity)
{
	const int32 Added = FMath::Min(FMath::Max(Quantity, 0), GetRoomFor(Item));
	if (Added > 0)
	{
		const int32 NewCount = (Items.FindOrAdd(Item) += Added);
		OnChanged.Broadcast(Item, NewCount);
	}
	return Added;
}

int32 UMEInventoryComponent::Remove(UMEItemData* Item, int32 Quantity)
{
	int32* Count = Items.Find(Item);
	const int32 Removed = Count ? FMath::Min(FMath::Max(Quantity, 0), *Count) : 0;
	if (Removed > 0)
	{
		*Count -= Removed;
		OnChanged.Broadcast(Item, *Count);
	}
	return Removed;
}
