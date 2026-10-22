#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MEFirstPersonAnimInstance.generated.h"

class AMECharacter;
class UAnimSequenceBase;
class UMEWeaponData;

/**
 * Drives the first-person arms (view model).
 * The AnimBP (ABP_FirstPersonArms) blends the current weapon's loops from these values
 * and plays actions (draw, fire, reload...) on its slot. No logic in the graph.
 */
UCLASS()
class METROEXODUSLIKE_API UMEFirstPersonAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UMEFirstPersonAnimInstance();

	/** Switches the arm loops to this weapon's. */
	void SetWeaponData(const UMEWeaponData* WeaponData);

	/** Plays an arms action on the slot (draw, fire, reload...). */
	void PlayAction(UAnimSequenceBase* Animation);

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsSprinting = false;

	// Current weapon loops, bound to the sequence players of the graph
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UAnimSequenceBase> IdleLoop;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UAnimSequenceBase> WalkLoop;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UAnimSequenceBase> SprintLoop;

	/** Slot the actions play on. Must exist in the AnimGraph. */
	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	FName ActionSlot = TEXT("DefaultSlot");

private:
	UPROPERTY(Transient)
	TObjectPtr<AMECharacter> Character;
};
