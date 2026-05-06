#include "Player/TDPlayerController.h"
#include "TDTowerBase.h"
#include "TDGameState.h"
#include "TDGameMode.h"
#include "TDPlayerCharacter.h"
#include "TDPlayerPawn.h"
#include "TDTowerPawn.h"
#include "Server/TDTowerSpawnerComponent.h"
#include "UI/TDTowerActionWidgetBase.h"
#include "EnhancedInputComponent.h"
#include "Blueprint/UserWidget.h"

// ── 생명주기 ──────────────────────────────────────────────────────────────────

void ATDPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 이전 레벨(로비 등)에서 UIOnly 로 설정된 Viewport InputMode 를 게임 입력으로 초기화.
	// SetInputMode 는 PC 가 아닌 GameViewportClient 에 저장되므로
	// 레벨 전환 후 새 PC 가 생성돼도 명시적으로 리셋해야 함.
	bShowMouseCursor = true;
	SetInputMode(FInputModeGameAndUI());
}

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
	FHitResult HitResult;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		HideTowerActionMenu();
		return;
	}

	AActor* HitActor = HitResult.GetActor();

	// Tower 히트 → 액션 메뉴 표시 (BP_Player::SelectTower 대체)
	if (ATDTowerBase* Tower = Cast<ATDTowerBase>(HitActor))
	{
		ShowTowerActionMenu(Tower);
		return;
	}

	// Tower 가 아닌 곳 → 메뉴 숨김 + 기존 이동 로직
	HideTowerActionMenu();

	if (ATDPlayerCharacter* TDCharacter = Cast<ATDPlayerCharacter>(GetPawn()))
	{
		if (ATDPlayerPawn* PlayerPawn = TDCharacter->GetPlayerPawn())
		{
			PlayerPawn->SetMoveTarget(HitResult.Location);
		}
	}
}

// ── 타워 액션 (Client → Server RPC) ──────────────────────────────────────────

void ATDPlayerController::Server_DoTowerAction_Implementation(ATDTowerBase* Tower, ETowerActions Action)
{
	if (!IsValid(Tower)) return;

	// Upgrade는 PC에서 사전 검증 — Build*/BreakDown은 TowerSpawner.DoTowerAction 내부 검증에 위임
	if (Action == ETowerActions::Upgrade)
	{
		if (!Tower->CanUpgrade()) return;

		ATDGameState* GS = GetWorld()->GetGameState<ATDGameState>();
		if (!GS || !GS->HasCoins(Tower->GetUpgradeCost())) return;
	}

	// 액션 실행은 서버 권위 컴포넌트(UTDTowerSpawnerComponent)로 위임
	if (ATDGameMode* GM = GetWorld()->GetAuthGameMode<ATDGameMode>())
	{
		if (GM->TowerSpawner)
		{
			GM->TowerSpawner->DoTowerAction(Tower, Action);
		}
	}
}

// ── 타워 액션 메뉴 (LocalPlayer UI) ────────────────────────────────────────────

void ATDPlayerController::ShowTowerActionMenu(ATDTowerBase* Tower)
{
	if (!IsValid(Tower)) return;

	const bool bIsMenuOpen = ActiveActionWidget &&
		ActiveActionWidget->GetVisibility() == ESlateVisibility::Visible;

	// 같은 Tower 재클릭 → 토글 닫힘
	if (bIsMenuOpen && SelectedTower == Tower)
	{
		HideTowerActionMenu();
		return;
	}

	// 이전 Tower 의 OnDestroyed 구독 해제
	if (IsValid(SelectedTower) && SelectedTower != Tower)
	{
		SelectedTower->OnDestroyed.RemoveDynamic(this, &ATDPlayerController::HandleSelectedTowerDestroyed);
	}

	SelectedTower = Tower;
	SelectedTower->OnDestroyed.AddUniqueDynamic(this, &ATDPlayerController::HandleSelectedTowerDestroyed);

	// 위젯 첫 호출 시 생성 + 캐시
	if (!ActiveActionWidget)
	{
		UClass* WidgetClass = ActionWidgetClass.LoadSynchronous();
		if (!WidgetClass)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[PC] ActionWidgetClass not set — assign WBP_TowerActions in BP_TDPlayerController defaults."));
			return;
		}

		// CreateWidget(this) → owning player = 현재 PC 의 LocalPlayer
		ActiveActionWidget = CreateWidget<UTDTowerActionWidgetBase>(this, WidgetClass);
		if (!ActiveActionWidget) return;

		// AddToPlayerScreen — PIE 다중 플레이어에서 다른 창에 안 보임 (리슨 서버 안전)
		ActiveActionWidget->AddToPlayerScreen(/*ZOrder=*/100);
	}

	ActiveActionWidget->InitForTower(Tower);
	ActiveActionWidget->SetVisibility(ESlateVisibility::Visible);
}

void ATDPlayerController::HideTowerActionMenu()
{
	if (IsValid(SelectedTower))
	{
		SelectedTower->OnDestroyed.RemoveDynamic(this, &ATDPlayerController::HandleSelectedTowerDestroyed);
	}
	SelectedTower = nullptr;

	if (ActiveActionWidget)
	{
		// 파괴하지 않고 숨김 — 다음 클릭 시 즉시 재사용
		ActiveActionWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ATDPlayerController::HandleSlotClicked(ETowerActions Action)
{
	if (!IsValid(SelectedTower) || Action == ETowerActions::None) return;
	Server_DoTowerAction(SelectedTower, Action);
}

void ATDPlayerController::HandleSelectedTowerDestroyed(AActor* /*DestroyedActor*/)
{
	HideTowerActionMenu();
}
