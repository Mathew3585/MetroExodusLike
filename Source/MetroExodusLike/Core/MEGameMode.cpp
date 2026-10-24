#include "Core/MEGameMode.h"
#include "Player/MECharacter.h"
#include "Player/MEPlayerController.h"
#include "UI/MEHUD.h"

AMEGameMode::AMEGameMode()
{
	DefaultPawnClass = AMECharacter::StaticClass();
	PlayerControllerClass = AMEPlayerController::StaticClass();
	HUDClass = AMEHUD::StaticClass();
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
