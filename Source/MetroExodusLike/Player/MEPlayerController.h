#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MEPlayerController.generated.h"

class UMEInteractionComponent;
class UMEWeaponComponent;
class UMEInteractionPromptWidget;
class UMECrosshairWidget;
class UMECrosshairData;
struct FMEInteractionInfo;

/**
 * Player controller. Owns the UI (interaction prompt, crosshair, later menus).
 */
UCLASS()
class METROEXODUSLIKE_API AMEPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** Layout of the interaction prompt (WBP_InteractionPrompt). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMEInteractionPromptWidget> InteractionPromptClass;

	/** Layout of the crosshair (WBP_Crosshair). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMECrosshairWidget> CrosshairClass;

	/** Crosshair of the whole game. A weapon can set its own. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UMECrosshairData> DefaultCrosshair;

private:
	void HandleInteractionFocusChanged(bool bHasFocus, const FMEInteractionInfo& Info);
	void HandleInteractionProgress(float Progress);
	void HandleTargetHit(bool bKilled);

	/** Display name of the key bound to Interact ("E"), so the prompt follows rebinding. */
	FText GetInteractKeyText() const;

	UPROPERTY(Transient)
	TObjectPtr<UMEInteractionPromptWidget> InteractionPrompt;

	UPROPERTY(Transient)
	TObjectPtr<UMECrosshairWidget> Crosshair;

	TWeakObjectPtr<UMEInteractionComponent> BoundInteraction;
	TWeakObjectPtr<UMEWeaponComponent> BoundWeapons;
};
