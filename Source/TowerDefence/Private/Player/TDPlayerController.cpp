#include "Player/TDPlayerController.h"
#include "TDTowerBase.h"

void ATDPlayerController::Server_DoTowerAction_Implementation(ATDTowerBase* Tower, ETowerActions Action)
{
	if (!IsValid(Tower)) return;

	Tower->DoTowerAction(Action);
}
