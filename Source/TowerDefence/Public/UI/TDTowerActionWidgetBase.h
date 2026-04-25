#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TowerDefence/TD.h"
#include "TDTowerActionWidgetBase.generated.h"

class ATDTowerBase;

/**
 * 타워 액션 위젯 (Build/Upgrade/BreakDown) 의 C++ 베이스.
 * - BP 자식 (예: WBP_TowerActions) 이 시각/레이아웃 담당.
 * - C++ 베이스가 데이터/로직/RPC 위임 담당.
 * - 책임 분리:
 *     · 슬롯 4종(Top/Bottom/Left/Right) 의 ETowerActions 매핑은 BP CDO 에서 EditDefaults
 *     · 비용/설명 계산은 ATDTowerBase::GetTowerDetails 호출
 *     · 코인 변동 시 자동 갱신 (OnCoinsChanged 바인딩)
 *     · 버튼 클릭 → PlayerController Server RPC 위임
 */
UCLASS(Abstract)
class TOWERDEFENCE_API UTDTowerActionWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * HUD/PlayerController 가 호출. 대상 타워 결정 + 비용 갱신 + 코인 이벤트 바인딩.
	 * @param Tower 액션을 적용할 대상 타워. nullptr 이면 무시.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerAction")
	void InitForTower(ATDTowerBase* Tower);

	/**
	 * 슬롯 버튼이 BP OnClicked 에서 호출. PlayerController Server RPC 로 위임.
	 * 리슨 서버 호스트도 동일 경로 (PC RPC) 사용.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerAction")
	void RequestTowerAction(ETowerActions Action);

protected:
	virtual void NativeDestruct() override;

	/** 4 슬롯의 액션 종류 — BP CDO 에서 설정. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TD|TowerAction|Slots")
	ETowerActions ActionTop = ETowerActions::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TD|TowerAction|Slots")
	ETowerActions ActionBottom = ETowerActions::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TD|TowerAction|Slots")
	ETowerActions ActionLeft = ETowerActions::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TD|TowerAction|Slots")
	ETowerActions ActionRight = ETowerActions::None;

	/** 현재 표시 중인 타워. */
	UPROPERTY(BlueprintReadOnly, Category = "TD|TowerAction")
	TObjectPtr<ATDTowerBase> TargetTower;

	/**
	 * 슬롯 갱신 이벤트. BP 자식이 override 하여 텍스처/스타일/Visibility 적용.
	 * RefreshAllSlots() 호출 시 4번 호출됨 (Top/Bottom/Left/Right 각 1회).
	 *
	 * @param Action       해당 슬롯의 액션 (None 이면 슬롯 미사용 — bVisible=false 로 호출)
	 * @param Cost         GetTowerDetails 결과 비용/환급액
	 * @param Description  GetTowerDetails 결과 설명 텍스트
	 * @param bVisible     슬롯 표시 여부 (예: Upgrade 가 불가능하면 false)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "TD|TowerAction")
	void OnSlotRefreshed(ETowerActions Action, int32 Cost, const FString& Description, bool bVisible);

private:
	/** 4 슬롯 모두 갱신. */
	void RefreshAllSlots();

	/** 단일 슬롯 갱신 → OnSlotRefreshed 발화. */
	void RefreshSlot(ETowerActions Action);

	UFUNCTION()
	void HandleCoinsChanged(int32 Change, int32 Coin);

	/** AddDynamic 으로 바인딩한 GameState 추적 (Destruct 시 RemoveDynamic 용). */
	UPROPERTY(Transient)
	TWeakObjectPtr<class ATDGameState> BoundGameState;
};
