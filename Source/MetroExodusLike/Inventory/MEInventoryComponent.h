#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MEInventoryComponent.generated.h"

class UMEItemData;

/** An item count changed (picked up, used, crafted...). */
DECLARE_MULTICAST_DELEGATE_TwoParams(FMEOnInventoryChanged, const UMEItemData* /*Item*/, int32 /*NewCount*/);

/**
 * What the player carries: a count per item, capped by each item's MaxCarry.
 * Ammo lives here too (per caliber), the weapons only hold their loaded magazine.
 */
UCLASS(ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class METROEXODUSLIKE_API UMEInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMEInventoryComponent();

	int32 GetCount(const UMEItemData* Item) const;

	/** How many more of this item fit. */
	int32 GetRoomFor(const UMEItemData* Item) const;

	/** Adds up to Quantity (capped by MaxCarry). Returns how many were added. */
	int32 Add(UMEItemData* Item, int32 Quantity);

	/** Removes up to Quantity. Returns how many were removed. */
	int32 Remove(UMEItemData* Item, int32 Quantity);

	FMEOnInventoryChanged OnChanged;

protected:
	virtual void BeginPlay() override;

	/** What the player starts with. */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TMap<TObjectPtr<UMEItemData>, int32> StartingItems;

private:
	UPROPERTY(Transient)
	TMap<TObjectPtr<UMEItemData>, int32> Items;
};
