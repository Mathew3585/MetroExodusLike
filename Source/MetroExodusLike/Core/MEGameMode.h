#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MEGameMode.generated.h"

/**
 * Main game mode. Sets the default player character and controller.
 */
UCLASS()
class METROEXODUSLIKE_API AMEGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMEGameMode();

	/** The player's body is removed and a new one starts at a player start (later: the last checkpoint). */
	void RespawnPlayer(AController* Controller);

	bool ShouldShowDebugAtStart() const { return bShowDebug; }

protected:
	/** Debug panel on screen at start (health, medkits, inventory, weapon...). MEDebug toggles it in game. */
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool bShowDebug = true;
};
