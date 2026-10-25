#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/MEInteractable.h"
#include "MEDoor.generated.h"

/**
 * Hinged door. Swings away from the player who opens it.
 * The hinge is the actor origin; the door leaf extends along the actor's X axis.
 */
UCLASS()
class METROEXODUSLIKE_API AMEDoor : public AActor, public IMEInteractable
{
	GENERATED_BODY()

public:
	AMEDoor();

	virtual void Tick(float DeltaSeconds) override;

	// IMEInteractable
	virtual FMEInteractionInfo GetInteractionInfo(const AMECharacter* Interactor) const override;
	virtual void Interact(AMECharacter* Interactor) override;

	bool IsOpen() const { return bIsOpen; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Hinge;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	UPROPERTY(EditAnywhere, Category = "Door", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float OpenAngle = 95.f;

	/** Swing speed (higher = faster, eases out at the end). */
	UPROPERTY(EditAnywhere, Category = "Door", meta = (ClampMin = "0"))
	float SwingSpeed = 4.f;

	/** Locked doors show "Locked" and cannot be opened. */
	UPROPERTY(EditAnywhere, Category = "Door")
	bool bLocked = false;

	/** How far enemies hear it open or close (share of their hearing range). */
	UPROPERTY(EditAnywhere, Category = "Door", meta = (ClampMin = "0"))
	float Noise = 0.3f;

private:
	bool bIsOpen = false;
	float CurrentAngle = 0.f;
	float TargetAngle = 0.f;
};
