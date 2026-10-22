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
};
