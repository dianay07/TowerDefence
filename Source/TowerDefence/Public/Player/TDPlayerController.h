#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TowerDefence/TD.h"
#include "TDPlayerController.generated.h"

class ATDTowerBase;
class ATDGameState;
class UInputAction;
class ATDTowerPawn;

/**
 * TD 전용 PlayerController.
 * - 클라이언트 → 서버 요청의 단일 진입점 (CLAUDE.md §1-3).
 * - 모든 Server RPC는 여기서 검증 후 서버 로직에 위임.
 */
UCLASS()
class TOWERDEFENCE_API ATDPlayerController : public APlayerController
{
	GENERATED_BODY()

// ── 생명주기 ──────────────────────────────────────────────────────────────────
protected:
	virtual void SetupInputComponent() override;

// ── 입력 ──────────────────────────────────────────────────────────────────────
public:
	/** BP_PlayerController Details 에서 IA_Click 에셋 연결 필요. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* ClickAction;

	/** ClickAction 트리거 시 호출. 타워 위 클릭이면 이동 무시. */
	void HandleClick();

// ── 타워 액션 (Client → Server RPC) ──────────────────────────────────────────
public:
	/**
	 * 클라이언트에서 호출 → 서버에서 실행.
	 * Tower->DoTowerAction(Action) 의 유일한 클라이언트 진입점.
	 * Upgrade: CanUpgrade + HasCoins 사전 검증. Build*/BreakDown: DoTowerAction 내부 검증에 위임.
	 * @param Tower  대상 타워 (nullptr 이면 무시)
	 * @param Action 수행할 액션 (Build / Upgrade / BreakDown)
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "TD|TowerAction")
	void Server_DoTowerAction(ATDTowerBase* Tower, ETowerActions Action);
};
