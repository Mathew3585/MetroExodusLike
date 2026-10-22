#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MEPlayerController.generated.h"

/**
 * Player controller. Will own input mapping setup and UI (menus, prompts).
 */
UCLASS()
class METROEXODUSLIKE_API AMEPlayerController : public APlayerController
{
	GENERATED_BODY()
};
