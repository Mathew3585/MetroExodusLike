#include "Core/MEGameMode.h"
#include "Player/MECharacter.h"
#include "Player/MEPlayerController.h"

AMEGameMode::AMEGameMode()
{
	DefaultPawnClass = AMECharacter::StaticClass();
	PlayerControllerClass = AMEPlayerController::StaticClass();
}

void AMEGameMode::RespawnPlayer(AController* Controller)
{
	if (!Controller)
	{
		return;
	}

	if (APawn* OldPawn = Controller->GetPawn())
	{
		Controller->UnPossess();
		OldPawn->Destroy();
	}
	RestartPlayer(Controller);
}
