#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MEHUD.generated.h"

/**
 * Draws the debug panel (health, medkits, inventory, weapon, movement) until the game has its own feedback
 * (watch, backpack menu). Shown at start if the game mode says so, toggled with the MEDebug command.
 * The game UI itself (prompt, crosshair) is made of widgets owned by the player controller.
 */
UCLASS()
class METROEXODUSLIKE_API AMEHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	void SetDebugShown(bool bShown) { bShowDebugPanel = bShown; }
	bool IsDebugShown() const { return bShowDebugPanel; }

protected:
	virtual void BeginPlay() override;

	/** Top-left corner of the panel, in pixels. */
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	FVector2D PanelPosition = FVector2D(20.f, 20.f);

	UPROPERTY(EditDefaultsOnly, Category = "Debug", meta = (ClampMin = "0.5"))
	float TextScale = 1.2f;

private:
	void DrawDebugPanel();

	/** Detection gauge above each enemy, "?" when it just heard something. */
	void DrawEnemyPerception();

	bool bShowDebugPanel = false;
};
