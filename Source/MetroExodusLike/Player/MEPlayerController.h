#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MEPlayerController.generated.h"

class UMEInteractionComponent;
class UMEInteractionPromptWidget;
struct FMEInteractionInfo;

/**
 * Player controller. Owns the UI (interaction prompt, later menus).
 */
UCLASS()
class METROEXODUSLIKE_API AMEPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	void HandleInteractionFocusChanged(bool bHasFocus, const FMEInteractionInfo& Info);
	void HandleInteractionProgress(float Progress);

	/** Display name of the key bound to Interact ("E"), so the prompt follows rebinding. */
	FText GetInteractKeyText() const;

	UPROPERTY(Transient)
	TObjectPtr<UMEInteractionPromptWidget> InteractionPrompt;

	TWeakObjectPtr<UMEInteractionComponent> BoundInteraction;
};
