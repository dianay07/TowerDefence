#include "Player/TDPlayerController.h"
#include "TDTowerBase.h"
#include "TDGameState.h"
#include "TDPlayerCharacter.h"
#include "TDPlayerPawn.h"
#include "TDTowerPawn.h"
#include "EnhancedInputComponent.h"

// ── 생명주기 ──────────────────────────────────────────────────────────────────

void ATDPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EIC->BindAction(ClickAction, ETriggerEvent::Started, this, &ATDPlayerController::HandleClick);
	}
}

// ── 입력 ──────────────────────────────────────────────────────────────────────

void ATDPlayerController::HandleClick()
{
	ATDPlayerCharacter* TDCharacter = Cast<ATDPlayerCharacter>(GetPawn());
	if (!TDCharacter) return;

	ATDPlayerPawn* PlayerPawn = TDCharacter->GetPlayerPawn();
	if (!PlayerPawn) return;

	FHitResult HitResult;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, HitResult)) return;

	// 타워 위 클릭은 이동 입력으로 처리하지 않음 (타워 UI 상호작용과 구분)
	if (HitResult.GetActor() && HitResult.GetActor()->IsA<ATDTowerPawn>()) return;

	PlayerPawn->SetMoveTarget(HitResult.Location);
}

// ── 타워 액션 (Client → Server RPC) ──────────────────────────────────────────

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
