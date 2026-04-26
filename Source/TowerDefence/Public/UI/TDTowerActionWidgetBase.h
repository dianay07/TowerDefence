#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TowerDefence/TD.h"
#include "TDTowerActionWidgetBase.generated.h"

class ATDTowerBase;

/**
 * 타워 액션 위젯 (Build / Upgrade / BreakDown) 의 C++ 베이스.
 *
 * 책임 분리:
 *   - C++ 베이스: 데이터 바인딩, 비용 계산, Server RPC 위임 (로직)
 *   - BP 자식 (WBP_TowerActions): UMG 레이아웃, 텍스처, 애니메이션 (시각)
 *
 * 슬롯 구조:
 *   - 4방향(Top/Bottom/Left/Right) 슬롯 각각에 ETowerActions 를 BP CDO 에서 매핑
 *   - ATDTowerBase::GetTowerDetails 로 비용/설명 조회
 *   - ATDGameState::OnCoinsChanged 바인딩 → 코인 변동 시 자동 갱신
 */
UCLASS(Abstract)
class TOWERDEFENCE_API UTDTowerActionWidgetBase : public UUserWidget
{
	GENERATED_BODY()

// ── 공개 API ──────────────────────────────────────────────────────────────────
public:
	/**
	 * HUD / PlayerController 에서 호출. 대상 타워 설정 + 비용 갱신 + 코인 이벤트 바인딩.
	 * @param Tower 액션을 적용할 대상 타워. nullptr 이면 무시.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerAction")
	void InitForTower(ATDTowerBase* Tower);

	/** 메뉴 표시 — Visibility = Visible. PC 또는 BP 에서 호출. */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerAction")
	void Show();

	/** 메뉴 숨김 — Visibility = Collapsed. 위젯 인스턴스는 보존 (재사용). */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerAction")
	void Hide();

	/** 4 슬롯 강제 재갱신 — 외부에서 트리거 필요 시 (예: 타워 업그레이드 직후). */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerAction")
	void RefreshSlots();

	/**
	 * 슬롯 버튼 BP OnClicked 에서 호출 → PlayerController Server RPC 위임.
	 * 리슨 서버 호스트도 동일 경로(PC RPC) 사용.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerAction")
	void RequestTowerAction(ETowerActions Action);

	// ─── 편의 wrapper — BP 슬롯 OnSlotClicked 이벤트 디스패처에서 직접 호출 ───
	// (BP 가 매번 ActionTop/Bottom/Left/Right 변수 읽고 RequestTowerAction 부르는 수고 제거)

	UFUNCTION(BlueprintCallable, Category = "TD|TowerAction")
	void RequestActionTop()    { RequestTowerAction(ActionTop); }

	UFUNCTION(BlueprintCallable, Category = "TD|TowerAction")
	void RequestActionBottom() { RequestTowerAction(ActionBottom); }

	UFUNCTION(BlueprintCallable, Category = "TD|TowerAction")
	void RequestActionLeft()   { RequestTowerAction(ActionLeft); }

	UFUNCTION(BlueprintCallable, Category = "TD|TowerAction")
	void RequestActionRight()  { RequestTowerAction(ActionRight); }

	/**
	 * 슬롯 위젯 → 해당 슬롯의 액션 조회.
	 * BP OnSlotRefreshed override 등에서 SlotWidget 인자로 액션 식별 시 사용.
	 */
	UFUNCTION(BlueprintPure, Category = "TD|TowerAction")
	ETowerActions GetActionForSlot(const UUserWidget* SlotWidget) const;

// ── 생명주기 ──────────────────────────────────────────────────────────────────
protected:
	virtual void NativeDestruct() override;

// ── BP 슬롯 설정 (BP CDO 에서 EditDefaults) ───────────────────────────────────
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TD|TowerAction|Slots")
	ETowerActions ActionTop    = ETowerActions::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TD|TowerAction|Slots")
	ETowerActions ActionBottom = ETowerActions::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TD|TowerAction|Slots")
	ETowerActions ActionLeft   = ETowerActions::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TD|TowerAction|Slots")
	ETowerActions ActionRight  = ETowerActions::None;

	/**
	 * 4 슬롯 위젯 인스턴스 (BindWidget).
	 * BP 자식 (WBP_TowerActions) 은 동일 이름의 위젯을 반드시 가져야 함 — 없으면 컴파일 에러.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "TD|TowerAction|Slots", meta = (BindWidget))
	TObjectPtr<UUserWidget> ActionSlotTop;

	UPROPERTY(BlueprintReadOnly, Category = "TD|TowerAction|Slots", meta = (BindWidget))
	TObjectPtr<UUserWidget> ActionSlotBottom;

	UPROPERTY(BlueprintReadOnly, Category = "TD|TowerAction|Slots", meta = (BindWidget))
	TObjectPtr<UUserWidget> ActionSlotLeft;

	UPROPERTY(BlueprintReadOnly, Category = "TD|TowerAction|Slots", meta = (BindWidget))
	TObjectPtr<UUserWidget> ActionSlotRight;

	/** 현재 위젯이 표시 중인 타워. InitForTower 에서 설정. */
	UPROPERTY(BlueprintReadOnly, Category = "TD|TowerAction")
	TObjectPtr<ATDTowerBase> TargetTower;

// ── BP 인터페이스 (BP 자식이 구현) ────────────────────────────────────────────
protected:
	/**
	 * 슬롯 갱신 이벤트. BP 자식이 override 하여 텍스처/스타일/Visibility 적용.
	 * RefreshAllSlots() 가 4방향 슬롯마다 1회씩 호출.
	 *
	 * @param SlotWidget   갱신 대상 슬롯 위젯 (ActionSlotTop/Bottom/Left/Right 중 하나)
	 * @param Action       해당 슬롯의 액션 (None 이면 슬롯 미사용 — bVisible=false 로 호출)
	 * @param Cost         GetTowerDetails 결과 비용 / 환급액
	 * @param Description  GetTowerDetails 결과 설명 텍스트
	 * @param bVisible     슬롯 표시 여부 (Upgrade 불가 등 조건에서 false)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "TD|TowerAction")
	void OnSlotRefreshed(UUserWidget* SlotWidget, ETowerActions Action, int32 Cost, const FString& Description, bool bVisible);

// ── 내부 구현 ─────────────────────────────────────────────────────────────────
private:
	/** 4 슬롯(Top/Bottom/Left/Right) 모두 갱신. */
	void RefreshAllSlots();

	/** 단일 슬롯 갱신 → OnSlotRefreshed 발화. None 또는 TargetTower 무효이면 bVisible=false. */
	void RefreshSlot(UUserWidget* SlotWidget, ETowerActions Action);

	UFUNCTION()
	void HandleCoinsChanged(int32 Change, int32 Coin);

	/** AddDynamic 으로 바인딩한 GameState 추적 → NativeDestruct 에서 RemoveDynamic 에 사용. */
	UPROPERTY(Transient)
	TWeakObjectPtr<class ATDGameState> BoundGameState;
};
