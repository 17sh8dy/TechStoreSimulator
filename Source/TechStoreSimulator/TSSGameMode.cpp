#include "TSSGameMode.h"
#include "TSSCharacter.h"
#include "TSSPlayerController.h"

ATSSGameMode::ATSSGameMode()
{
	DefaultPawnClass = ATSSCharacter::StaticClass();
	PlayerControllerClass = ATSSPlayerController::StaticClass();
}