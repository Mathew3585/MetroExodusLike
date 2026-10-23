#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MEItemData.generated.h"

class UStaticMesh;

UENUM(BlueprintType)
enum class EMEItemCategory : uint8
{
	/** Rounds of one caliber, shared by every weapon using it. */
	Ammo,
	/** Crafting material (common, technical...). */
	Resource,
	/** Used up: filters, medkits... */
	Consumable
};

/**
 * Everything that defines a kind of item. One asset per item (DA_Item_Ammo_556...),
 * the inventory counts how many the player carries.
 */
UCLASS(BlueprintType)
class METROEXODUSLIKE_API UMEItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, Category = "Item")
	EMEItemCategory Category = EMEItemCategory::Resource;

	/** Most the player can carry: pickups leave the rest on the ground. */
	UPROPERTY(EditDefaultsOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxCarry = 100;

	/** Mesh of the pickup lying in the world. */
	UPROPERTY(EditDefaultsOnly, Category = "Visuals")
	TObjectPtr<UStaticMesh> WorldMesh;
};
