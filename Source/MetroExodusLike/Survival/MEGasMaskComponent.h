#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MEGasMaskComponent.generated.h"

class AMECharacter;
class UMEItemData;
class USoundBase;
class UAudioComponent;

UENUM()
enum class EMEGasMaskState : uint8
{
	Off,
	PuttingOn,
	On,
	TakingOff,
	ChangingFilter
};

/**
 * Gas mask: put on / taken off with the left hand (the weapon goes away), filters with a timer.
 * A worn mask with filter time left protects from toxic air. Filters are inventory items; a used
 * filter drains whenever the mask is worn and is changed by hand (holding the mask key).
 * Breathing in the mask is heard all the time, heavier with exertion and when the filter runs low.
 */
UCLASS(ClassGroup = (Survival), meta = (BlueprintSpawnableComponent))
class METROEXODUSLIKE_API UMEGasMaskComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMEGasMaskComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Mask key: a tap puts on / takes off the mask, holding it changes the filter. */
	void MaskPressed();
	void MaskReleased();

	void ToggleMask();
	void ChangeFilter();

	EMEGasMaskState GetState() const { return State; }
	bool IsWorn() const { return State == EMEGasMaskState::On || State == EMEGasMaskState::TakingOff || State == EMEGasMaskState::ChangingFilter; }

	/** Time left on the filter in the mask. */
	float GetFilterRemaining() const { return FilterRemaining; }
	int32 GetSpareFilters() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Inventory item used up by each filter change. */
	UPROPERTY(EditDefaultsOnly, Category = "Gas Mask")
	TObjectPtr<UMEItemData> FilterItem;

	/** How long a filter lasts while the mask is worn. */
	UPROPERTY(EditDefaultsOnly, Category = "Gas Mask", meta = (ClampMin = "1", Units = "s"))
	float FilterDuration = 180.f;

	/** Holding the mask key this long changes the filter instead of taking the mask off. */
	UPROPERTY(EditDefaultsOnly, Category = "Gas Mask", meta = (ClampMin = "0.05", Units = "s"))
	float ChangeFilterHoldTime = 0.4f;

	/** Gesture times, the weapon out of view. The mask protects once it is on. */
	UPROPERTY(EditDefaultsOnly, Category = "Gas Mask|Gestures", meta = (ClampMin = "0", Units = "s"))
	float PutOnTime = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Gas Mask|Gestures", meta = (ClampMin = "0", Units = "s"))
	float TakeOffTime = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Gas Mask|Gestures", meta = (ClampMin = "0", Units = "s"))
	float ChangeFilterTime = 1.5f;

	/** Gesture sounds, played when the weapon is out of view. Optional. */
	UPROPERTY(EditDefaultsOnly, Category = "Gas Mask|Sound")
	TObjectPtr<USoundBase> PutOnSound;

	UPROPERTY(EditDefaultsOnly, Category = "Gas Mask|Sound")
	TObjectPtr<USoundBase> TakeOffSound;

	UPROPERTY(EditDefaultsOnly, Category = "Gas Mask|Sound")
	TObjectPtr<USoundBase> ChangeFilterSound;

	/** Breathing in the mask, at rest and out of breath: replayed back to back, blended by exertion. */
	UPROPERTY(EditDefaultsOnly, Category = "Gas Mask|Breathing")
	TObjectPtr<USoundBase> CalmBreathSound;

	UPROPERTY(EditDefaultsOnly, Category = "Gas Mask|Breathing")
	TObjectPtr<USoundBase> HardBreathSound;

	UPROPERTY(EditDefaultsOnly, Category = "Gas Mask|Breathing", meta = (ClampMin = "0"))
	float BreathVolume = 0.7f;

	/** Under this much filter time, breathing gets heavier: time to change it. */
	UPROPERTY(EditDefaultsOnly, Category = "Gas Mask|Breathing", meta = (ClampMin = "0", Units = "s"))
	float LowFilterTime = 30.f;

	/** How fast breathing follows exertion (blend per second). */
	UPROPERTY(EditDefaultsOnly, Category = "Gas Mask|Breathing", meta = (ClampMin = "0.1"))
	float BreathBlendSpeed = 1.5f;

private:
	/** Starts a gesture with the weapon away; Finish runs at its end. False if the hands are busy. */
	bool StartGesture(EMEGasMaskState GestureState, float Duration, USoundBase* Sound, void (UMEGasMaskComponent::*Finish)());
	void FinishPutOn();
	void FinishTakeOff();
	void FinishChangeFilter();

	void UpdateProtection();
	void StartBreathing();
	void StopBreathing();

	UFUNCTION()
	void OnCalmBreathFinished();

	UFUNCTION()
	void OnHardBreathFinished();

	UPROPERTY(Transient)
	TObjectPtr<AMECharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CalmBreath;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> HardBreath;

	EMEGasMaskState State = EMEGasMaskState::Off;
	float FilterRemaining = 0.f;
	float BreathEffort = 0.f;
	FTimerHandle GestureTimer;
	FTimerHandle HoldTimer;
};
