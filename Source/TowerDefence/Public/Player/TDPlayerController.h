#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TowerDefence/TD.h"
#include "TDPlayerController.generated.h"

class ATDTowerBase;

/**
 * TD 전용 PlayerController.
 * - 클라이언트 → 서버 요청의 단일 진입점 (CLAUDE.md §1-3).
 * - 모든 Server RPC는 여기서 검증 후 서버 로직에 위임.
 */
UCLASS()
class TOWERDEFENCE_API ATDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// ── 타워 액션 ─────────────────────────────────────────────────────────────

	/**
	 * 클라이언트에서 호출 → 서버에서 실행.
	 * Tower->DoTowerAction(Action) 의 유일한 클라이언트 진입점.
	 * @param Tower  대상 타워 (nullptr 이면 무시)
	 * @param Action 수행할 액션 (Build / Upgrade / BreakDown)
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "TD|TowerAction")
	void Server_DoTowerAction(ATDTowerBase* Tower, ETowerActions Action);
};
