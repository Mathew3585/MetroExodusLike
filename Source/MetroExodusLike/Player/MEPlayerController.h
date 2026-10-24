#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MEPlayerController.generated.h"

class UMEInteractionComponent;
class UMEWeaponComponent;
class UMEHealthComponent;
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

public:
	// Console commands for testing (PIE: ` then the command)

	/** MEDamage 25: hurts the player. */
	UFUNCTION(Exec)
	void MEDamage(float Amount);

	/** MEKill: kills the player. */
	UFUNCTION(Exec)
	void MEKill();

	/** MEHeal 50: gives health back. */
	UFUNCTION(Exec)
	void MEHeal(float Amount);

	/** MEDebug: shows / hides the debug panel. */
	UFUNCTION(Exec)
	void MEDebug();

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
	void HandlePlayerDeath(AController* Killer);

	/** Display name of the key bound to Interact ("E"), so the prompt follows rebinding. */
	FText GetInteractKeyText() const;

	UPROPERTY(Transient)
	TObjectPtr<UMEInteractionPromptWidget> InteractionPrompt;

	UPROPERTY(Transient)
	TObjectPtr<UMECrosshairWidget> Crosshair;

	TWeakObjectPtr<UMEInteractionComponent> BoundInteraction;
	TWeakObjectPtr<UMEWeaponComponent> BoundWeapons;
	TWeakObjectPtr<UMEHealthComponent> BoundHealth;
};
