#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/MEInteractable.h"
#include "MEPickup.generated.h"

class UMEItemData;

/**
 * A stack of items lying in the world (ammo, resources...). Picking it up adds it to the inventory;
 * what does not fit (MaxCarry) stays on the ground.
 */
UCLASS()
class METROEXODUSLIKE_API AMEPickup : public AActor, public IMEInteractable
{
	GENERATED_BODY()

public:
	AMEPickup();

	virtual void OnConstruction(const FTransform& Transform) override;

	// IMEInteractable
	virtual FMEInteractionInfo GetInteractionInfo(const AMECharacter* Interactor) const override;
	virtual void Interact(AMECharacter* Interactor) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** What lies here. Its world mesh is shown unless the Blueprint sets another one. */
	UPROPERTY(EditAnywhere, Category = "Pickup")
	TObjectPtr<UMEItemData> Item;

	UPROPERTY(EditAnywhere, Category = "Pickup", meta = (ClampMin = "1"))
	int32 Quantity = 1;
};
