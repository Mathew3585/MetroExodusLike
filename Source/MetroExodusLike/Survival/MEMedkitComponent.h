#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MEMedkitComponent.generated.h"

class AMECharacter;
class UMEItemData;
class USoundBase;

/**
 * Medkit on a key: the weapon goes away, the injection, health comes back over a short time.
 * Medkits are inventory items, used one at a time and only when hurt.
 */
UCLASS(ClassGroup = (Survival), meta = (BlueprintSpawnableComponent))
class METROEXODUSLIKE_API UMEMedkitComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMEMedkitComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Uses a medkit if the player carries one, is hurt and is not already healing. */
	void UseMedkit();

	/** From the key press to the end of the heal. */
	bool IsInUse() const { return bInjecting || HealRemaining > 0.f; }

protected:
	virtual void BeginPlay() override;

	/** Inventory item used up by each injection. */
	UPROPERTY(EditDefaultsOnly, Category = "Medkit")
	TObjectPtr<UMEItemData> MedkitItem;

	UPROPERTY(EditDefaultsOnly, Category = "Medkit", meta = (ClampMin = "0"))
	float HealAmount = 50.f;

	/** Health comes back over this time after the injection, not at once. */
	UPROPERTY(EditDefaultsOnly, Category = "Medkit", meta = (ClampMin = "0", Units = "s"))
	float HealTime = 1.5f;

	/** Time the weapon stays out of view for the injection. */
	UPROPERTY(EditDefaultsOnly, Category = "Medkit", meta = (ClampMin = "0", Units = "s"))
	float InjectTime = 0.8f;

	/** Played when the weapon is out of view. Optional. */
	UPROPERTY(EditDefaultsOnly, Category = "Medkit")
	TObjectPtr<USoundBase> InjectSound;

private:
	void Inject();

	UPROPERTY(Transient)
	TObjectPtr<AMECharacter> Character;

	bool bInjecting = false;
	float HealRemaining = 0.f;
	FTimerHandle InjectTimer;
};
