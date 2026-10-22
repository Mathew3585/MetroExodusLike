#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/MEInteractable.h"
#include "MEInteractionComponent.generated.h"

class AMECharacter;

DECLARE_MULTICAST_DELEGATE_TwoParams(FMEOnInteractionFocusChanged, bool /*bHasFocus*/, const FMEInteractionInfo& /*Info*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FMEOnInteractionProgress, float /*Progress 0..1*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FMEOnInteractionStarted, EMEInteractionGesture /*Gesture*/);

/**
 * Finds what the player looks at (trace from the camera), keeps it in focus,
 * and runs the interaction: instant, or held for the interactable's duration.
 * The UI and the gesture animation listen to its delegates.
 */
UCLASS(ClassGroup = (Interaction), meta = (BlueprintSpawnableComponent))
class METROEXODUSLIKE_API UMEInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMEInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Interact key pressed. */
	void BeginInteract();

	/** Interact key released: cancels a hold in progress. */
	void EndInteract();

	FMEOnInteractionFocusChanged OnFocusChanged;
	FMEOnInteractionProgress OnProgress;
	FMEOnInteractionStarted OnInteractionStarted;

protected:
	virtual void BeginPlay() override;

	/** How far the player can reach. */
	UPROPERTY(EditAnywhere, Category = "Interaction", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 180.f;

	/** Radius of the trace: a bit forgiving so small items are easy to target. */
	UPROPERTY(EditAnywhere, Category = "Interaction", meta = (ClampMin = "0", Units = "cm"))
	float TraceRadius = 6.f;

private:
	void UpdateFocus();
	void SetFocus(AActor* NewFocus);
	void BroadcastFocus();
	void CompleteInteraction();
	void CancelHold();

	IMEInteractable* GetFocusedInteractable() const;

	UPROPERTY(Transient)
	TObjectPtr<AMECharacter> Character;

	TWeakObjectPtr<AActor> FocusedActor;

	/** Last state sent to listeners, to only broadcast changes. */
	FMEInteractionInfo LastInfo;
	bool bHadFocus = false;

	bool bIsHolding = false;
	float HoldElapsed = 0.f;
	float HoldDuration = 0.f;
};
