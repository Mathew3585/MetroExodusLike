#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/MEInteractable.h"
#include "MEPickup.generated.h"

/**
 * Item lying in the world that the player picks up.
 * Goes to the inventory once it exists (Phase 7); for now it is logged and removed.
 */
UCLASS()
class METROEXODUSLIKE_API AMEPickup : public AActor, public IMEInteractable
{
	GENERATED_BODY()

public:
	AMEPickup();

	// IMEInteractable
	virtual FMEInteractionInfo GetInteractionInfo(const AMECharacter* Interactor) const override;
	virtual void Interact(AMECharacter* Interactor) override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, Category = "Pickup")
	FText ItemName;

	UPROPERTY(EditAnywhere, Category = "Pickup", meta = (ClampMin = "1"))
	int32 Quantity = 1;
};
