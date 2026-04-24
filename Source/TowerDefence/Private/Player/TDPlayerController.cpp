#include "Player/TDPlayerController.h"
#include "TDTowerBase.h"
#include "TDGameState.h"
#include "TDPlayerCharacter.h"
#include "TDPlayerPawn.h"
#include "TDTowerPawn.h"
#include "EnhancedInputComponent.h"

void ATDPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EIC->BindAction(ClickAction, ETriggerEvent::Started, this, &ATDPlayerController::HandleClick);
	}
}

void ATDPlayerController::HandleClick()
{
	ATDPlayerCharacter* TDCharacter = Cast<ATDPlayerCharacter>(GetPawn());
	if (!TDCharacter) return;

	ATDPlayerPawn* PlayerPawn = TDCharacter->GetPlayerPawn();
	if (!PlayerPawn) return;

	FHitResult HitResult;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, HitResult)) return;

	if (HitResult.GetActor() && HitResult.GetActor()->IsA<ATDTowerPawn>()) return;

	PlayerPawn->SetMoveTarget(HitResult.Location);
}

void ATDPlayerController::Server_DoTowerAction_Implementation(ATDTowerBase* Tower, ETowerActions Action)
{
	if (!IsValid(Tower)) return;

	// Upgrade는 PC에서 사전 검증 — Build*/BreakDown은 DoTowerAction 내부 검증에 위임
	if (Action == ETowerActions::Upgrade)
	{
		if (!Tower->CanUpgrade()) return;

		ATDGameState* GS = GetWorld()->GetGameState<ATDGameState>();
		if (!GS || !GS->HasCoins(Tower->GetUpgradeCost())) return;
	}

	Tower->DoTowerAction(Action);
}
