#include "UI/TDTowerActionWidgetBase.h"
#include "TDTowerBase.h"
#include "TDGameState.h"
#include "Player/TDPlayerController.h"
#include "Kismet/GameplayStatics.h"

// ── 공개 API ──────────────────────────────────────────────────────────────────

void UTDTowerActionWidgetBase::InitForTower(ATDTowerBase* Tower)
{
	TargetTower = Tower;
	if (!IsValid(TargetTower)) return;

	// 코인 변경 시 자동 갱신 — GameState::OnCoinsChanged 바인딩
	if (ATDGameState* GS = Cast<ATDGameState>(UGameplayStatics::GetGameState(this)))
	{
		// 재초기화(다른 타워로 전환) 케이스: 이전 바인딩 먼저 해제
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

	// 리슨 서버 호스트 포함 모든 클라 → PlayerController Server RPC 경유
	if (ATDPlayerController* PC = Cast<ATDPlayerController>(GetOwningPlayer()))
	{
		PC->Server_DoTowerAction(TargetTower, Action);
	}
}

// ── 생명주기 ──────────────────────────────────────────────────────────────────

void UTDTowerActionWidgetBase::NativeDestruct()
{
	if (BoundGameState.IsValid())
	{
		BoundGameState->OnCoinsChanged.RemoveDynamic(this, &UTDTowerActionWidgetBase::HandleCoinsChanged);
		BoundGameState.Reset();
	}

	Super::NativeDestruct();
}

// ── 내부 구현 ─────────────────────────────────────────────────────────────────

void UTDTowerActionWidgetBase::RefreshAllSlots()
{
	RefreshSlot(ActionTop);
	RefreshSlot(ActionBottom);
	RefreshSlot(ActionLeft);
	RefreshSlot(ActionRight);
}

void UTDTowerActionWidgetBase::RefreshSlot(ETowerActions Action)
{
	// None 슬롯 또는 타워 무효 → 비표시
	if (Action == ETowerActions::None || !IsValid(TargetTower))
	{
		OnSlotRefreshed(Action, 0, FString(), /*bVisible=*/false);
		return;
	}

	int32 Cost = 0;
	FString Description;
	TargetTower->GetTowerDetails(Action, Cost, Description);

	// Upgrade 가 불가능한 타워는 슬롯 숨김 (최고 등급 등)
	const bool bVisible = !(Action == ETowerActions::Upgrade && !TargetTower->CanUpgrade());

	OnSlotRefreshed(Action, Cost, Description, bVisible);
}

void UTDTowerActionWidgetBase::HandleCoinsChanged(int32 /*Change*/, int32 /*Coin*/)
{
	// 코인 변경 시 4슬롯 비용/가용성 전체 재계산
	RefreshAllSlots();
}
