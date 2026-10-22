#include "Core/MEGameMode.h"
#include "Player/MECharacter.h"
#include "Player/MEPlayerController.h"

AMEGameMode::AMEGameMode()
{
	DefaultPawnClass = AMECharacter::StaticClass();
	PlayerControllerClass = AMEPlayerController::StaticClass();
}
