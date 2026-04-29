#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TowerDefence/TD.h"
#include "TDPlayerController.generated.h"

class ATDTowerBase;
class ATDGameState;
class UInputAction;
class ATDTowerPawn;
class UTDTowerActionWidgetBase;

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
	virtual void BeginPlay() override;
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
	 * Upgrade: CanUpgrade + HasCoins 사전 검증. Build
	 * @param Tower  대상 타워 (nullptr 이면 무시)
	 * @param Action 수행할 액션 (Build / Upgrade / BreakDown)
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "TD|TowerAction")
	void Server_DoTowerAction(ATDTowerBase* Tower, ETowerActions Action);

	// ── 타워 액션 메뉴 (LocalPlayer UI) ────────────────────────────────────────
	// 리슨 서버: 호스트/게스트 모두 각자의 PC 가 자기 LocalPlayer 측 메뉴를 관리.
	// Server RPC 는 Server_DoTowerAction 만. 메뉴 자체는 비복제 (UI 는 항상 로컬).

	/** Tower 클릭 시 호출 — 메뉴 표시. 같은 Tower 재클릭이면 토글 닫힘. */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerAction")
	void ShowTowerActionMenu(ATDTowerBase* Tower);

	/** 메뉴 숨김 — 빈 곳 클릭, Tower 파괴, ESC 등. */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerAction")
	void HideTowerActionMenu();

	/** 슬롯 위젯이 클릭 시 호출 — Server_DoTowerAction 위임. */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerAction")
	void HandleSlotClicked(ETowerActions Action);

protected:
	/** 위젯 클래스 — BP_TDPlayerController 에 WBP_TowerActions 지정. */
	UPROPERTY(EditDefaultsOnly, Category = "TD|TowerAction")
	TSoftClassPtr<UTDTowerActionWidgetBase> ActionWidgetClass;

private:
	/** 캐시된 위젯 인스턴스 — 첫 표시 시 생성, 이후 Visibility 만 토글. */
	UPROPERTY(Transient)
	TObjectPtr<UTDTowerActionWidgetBase> ActiveActionWidget;

	/** 현재 메뉴 표시 중인 Tower. */
	UPROPERTY(Transient)
	TObjectPtr<ATDTowerBase> SelectedTower;

	/** Tower 가 파괴되면 메뉴도 닫음. */
	UFUNCTION()
	void HandleSelectedTowerDestroyed(AActor* DestroyedActor);
};
