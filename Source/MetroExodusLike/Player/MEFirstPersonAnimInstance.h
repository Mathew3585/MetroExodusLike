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

	// Close to a wall: the weapon goes to the close pose, then the against pose. Blended over everything else.
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Wall")
	TObjectPtr<UAnimSequenceBase> CloseToWallPose;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Wall")
	TObjectPtr<UAnimSequenceBase> AgainstWallPose;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Wall")
	float CloseToWallAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Wall")
	float AgainstWallAlpha = 0.f;

	/** Slot the actions play on. Must exist in the AnimGraph. */
	UPROPERTY(EditDefaultsOnly, Category = "Animations")
	FName ActionSlot = TEXT("DefaultSlot");

private:
	UPROPERTY(Transient)
	TObjectPtr<AMECharacter> Character;
};
