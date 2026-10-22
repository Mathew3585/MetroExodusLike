#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/MEInteractable.h"
#include "MESwitch.generated.h"

/**
 * Light switch or breaker lever: toggles every light component of the linked actors.
 * With a hold duration it becomes a lever the player has to pull.
 */
UCLASS()
class METROEXODUSLIKE_API AMESwitch : public AActor, public IMEInteractable
{
	GENERATED_BODY()

public:
	AMESwitch();

	// IMEInteractable
	virtual FMEInteractionInfo GetInteractionInfo(const AMECharacter* Interactor) const override;
	virtual void Interact(AMECharacter* Interactor) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Actors whose light components this switch controls (picked in the level). */
	UPROPERTY(EditInstanceOnly, Category = "Switch")
	TArray<TObjectPtr<AActor>> ControlledLights;

	UPROPERTY(EditAnywhere, Category = "Switch")
	bool bIsOn = true;

	/** Seconds to hold the key. 0 = simple switch, > 0 = heavy lever / breaker. */
	UPROPERTY(EditAnywhere, Category = "Switch", meta = (ClampMin = "0", Units = "s"))
	float HoldDuration = 0.f;

private:
	void ApplyState();
};
