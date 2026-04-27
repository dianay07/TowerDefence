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

	InitAllSlots();      // 텍스처/스타일 등 정적 데이터 1회 세팅
	RefreshAllSlots();   // Cost/Visibility 등 동적 데이터 갱신
}

void UTDTowerActionWidgetBase::Show()
{
	SetVisibility(ESlateVisibility::Visible);
}

void UTDTowerActionWidgetBase::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UTDTowerActionWidgetBase::RefreshSlots()
{
	RefreshAllSlots();
}

void UTDTowerActionWidgetBase::RequestTowerAction(ETowerActions Action)
{
	if (Action == ETowerActions::None) return;

	// PC 가 SelectedTower 보유 + Server RPC 위임 단일 진입점 (리슨 서버 호스트 포함 모든 클라)
	if (ATDPlayerController* PC = Cast<ATDPlayerController>(GetOwningPlayer()))
	{
		PC->HandleSlotClicked(Action);
	}
}

ETowerActions UTDTowerActionWidgetBase::GetActionForSlot(const UUserWidget* SlotWidget) const
{
	if (SlotWidget == ActionSlotTop)    return ActionTop;
	if (SlotWidget == ActionSlotBottom) return ActionBottom;
	if (SlotWidget == ActionSlotLeft)   return ActionLeft;
	if (SlotWidget == ActionSlotRight)  return ActionRight;
	return ETowerActions::None;
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

void UTDTowerActionWidgetBase::InitAllSlots()
{
	InitSlot(ActionSlotTop,    ActionTop);
	InitSlot(ActionSlotBottom, ActionBottom);
	InitSlot(ActionSlotLeft,   ActionLeft);
	InitSlot(ActionSlotRight,  ActionRight);
}

void UTDTowerActionWidgetBase::InitSlot(UUserWidget* SlotWidget, ETowerActions Action)
{
	if (!SlotWidget) return;
	OnSlotInitialized(SlotWidget, Action);
}

void UTDTowerActionWidgetBase::RefreshAllSlots()
{
	RefreshSlot(ActionSlotTop,    ActionTop);
	RefreshSlot(ActionSlotBottom, ActionBottom);
	RefreshSlot(ActionSlotLeft,   ActionLeft);
	RefreshSlot(ActionSlotRight,  ActionRight);
}

void UTDTowerActionWidgetBase::RefreshSlot(UUserWidget* SlotWidget, ETowerActions Action)
{
	if (!SlotWidget) return;  // BP 가 BindWidget 누락 (보통 컴파일 단에서 잡힘)

	// None 슬롯 또는 타워 무효 → 비표시
	if (Action == ETowerActions::None || !IsValid(TargetTower))
	{
		OnSlotRefreshed(SlotWidget, Action, 0, FString(), /*bVisible=*/false);
		return;
	}

	int32 Cost = 0;
	FString Description;
	TargetTower->GetTowerDetails(Action, Cost, Description);

	// Upgrade 가 불가능한 타워는 슬롯 숨김 (최고 등급 등)
	const bool bVisible = !(Action == ETowerActions::Upgrade && !TargetTower->CanUpgrade());

	OnSlotRefreshed(SlotWidget, Action, Cost, Description, bVisible);
}

void UTDTowerActionWidgetBase::HandleCoinsChanged(int32 /*Change*/, int32 /*Coin*/)
{
	// 코인 변경 시 4슬롯 비용/가용성 전체 재계산
	RefreshAllSlots();
}
