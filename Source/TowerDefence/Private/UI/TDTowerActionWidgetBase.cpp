#include "UI/TDTowerActionWidgetBase.h"
#include "TDTowerBase.h"
#include "TDGameState.h"
#include "Player/TDPlayerController.h"
#include "Kismet/GameplayStatics.h"

void UTDTowerActionWidgetBase::InitForTower(ATDTowerBase* Tower)
{
	TargetTower = Tower;
	if (!IsValid(TargetTower)) return;

	// 코인 변경 시 자동 갱신을 위해 GameState OnCoinsChanged 바인딩
	if (ATDGameState* GS = Cast<ATDGameState>(UGameplayStatics::GetGameState(this)))
	{
		// 이전 바인딩이 남아있으면 정리 (재초기화 케이스)
		if (BoundGameState.IsValid())
		{
			BoundGameState->OnCoinsChanged.RemoveDynamic(this, &UTDTowerActionWidgetBase::HandleCoinsChanged);
		}

		GS->OnCoinsChanged.AddDynamic(this, &UTDTowerActionWidgetBase::HandleCoinsChanged);
		BoundGameState = GS;
	}

	RefreshAllSlots();
}

void UTDTowerActionWidgetBase::RequestTowerAction(ETowerActions Action)
{
	if (!IsValid(TargetTower) || Action == ETowerActions::None) return;

	// 모든 클라(리슨 서버 호스트 포함) → PlayerController Server RPC 경유
	if (ATDPlayerController* PC = Cast<ATDPlayerController>(GetOwningPlayer()))
	{
		PC->Server_DoTowerAction(TargetTower, Action);
	}
}

void UTDTowerActionWidgetBase::NativeDestruct()
{
	if (BoundGameState.IsValid())
	{
		BoundGameState->OnCoinsChanged.RemoveDynamic(this, &UTDTowerActionWidgetBase::HandleCoinsChanged);
		BoundGameState.Reset();
	}

	Super::NativeDestruct();
}

void UTDTowerActionWidgetBase::RefreshAllSlots()
{
	RefreshSlot(ActionTop);
	RefreshSlot(ActionBottom);
	RefreshSlot(ActionLeft);
	RefreshSlot(ActionRight);
}

void UTDTowerActionWidgetBase::RefreshSlot(ETowerActions Action)
{
	// None 슬롯은 비표시 처리 후 종료
	if (Action == ETowerActions::None || !IsValid(TargetTower))
	{
		OnSlotRefreshed(Action, 0, FString(), /*bVisible=*/false);
		return;
	}

	int32 Cost = 0;
	FString Description;
	TargetTower->GetTowerDetails(Action, Cost, Description);

	// Upgrade 가 불가능하면 슬롯 숨김
	bool bVisible = true;
	if (Action == ETowerActions::Upgrade && !TargetTower->CanUpgrade())
	{
		bVisible = false;
	}

	OnSlotRefreshed(Action, Cost, Description, bVisible);
}

void UTDTowerActionWidgetBase::HandleCoinsChanged(int32 /*Change*/, int32 /*Coin*/)
{
	// 코인 변경 시 4슬롯 비용/가용성 재계산
	RefreshAllSlots();
}
