#include "UI/TDWaitingRoomWidget.h"
#include "Session/TDLobbySessionSubsystem.h"
#include "Session/TDLevelSessionSubsystem.h"
#include "Lobby/TDLobbyPlayerController.h"
#include "Lobby/TDLobbyGameState.h"
#include "Lobby/TDLobbyPlayerState.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Blueprint/UserWidget.h"

// ── 생명주기 ──────────────────────────────────────────────────────────────────

void UTDWaitingRoomWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 버튼 바인딩
	if (StartButton)
		StartButton->OnClicked.AddDynamic(this, &UTDWaitingRoomWidget::OnStartClicked);

	if (LeaveButton)
		LeaveButton->OnClicked.AddDynamic(this, &UTDWaitingRoomWidget::OnLeaveClicked);

	// 호스트만 게임 시작 버튼 표시
	// ListenServer 에서 호스트 PC 는 NetConnection == nullptr
	if (StartButton)
	{
		const APlayerController* PC = GetOwningPlayer();
		const bool bIsHost = PC && (PC->GetNetConnection() == nullptr);
		StartButton->SetVisibility(bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// OnPlayerSlotsChanged 구독 후 초기 목록 갱신
	if (UWorld* World = GetWorld())
	{
		if (ATDLobbyGameState* GS = World->GetGameState<ATDLobbyGameState>())
		{
			GS->OnPlayerSlotsChanged.AddUniqueDynamic(this, &UTDWaitingRoomWidget::HandlePlayerSlotsChanged);
		}
	}
	RefreshPlayerList();

	// 맵 이름 표시
	// LobbySubsystem 의 MultiStageId → LevelSessionSubsystem 의 맵 경로 → 파일명만 추출
	if (Text_MapName)
	{
		FString DisplayName = TEXT("알 수 없는 맵");

		if (UTDLobbySessionSubsystem* Lobby = GetLobbySession())
		{
			const FName StageId = Lobby->GetMultiStageId();
			if (!StageId.IsNone())
			{
				if (UGameInstance* GI = GetGameInstance())
				{
					if (UTDLevelSessionSubsystem* LevelSub = GI->GetSubsystem<UTDLevelSessionSubsystem>())
					{
						// "/Game/Levels/Levels-01" → "Levels-01"
						const FString MapPath = LevelSub->GetMapPathByStageId(StageId);
						DisplayName = MapPath.IsEmpty()
							? StageId.ToString()
							: FPaths::GetBaseFilename(MapPath);
					}
				}
			}
		}

		Text_MapName->SetText(FText::FromString(DisplayName));
		UE_LOG(LogTemp, Log, TEXT("[WaitingRoom] 맵 이름 표시: %s"), *DisplayName);
	}

	UE_LOG(LogTemp, Log, TEXT("[WaitingRoom] NativeConstruct 완료."));
}

void UTDWaitingRoomWidget::NativeDestruct()
{
	// 델리게이트 구독 해제
	if (UWorld* World = GetWorld())
	{
		if (ATDLobbyGameState* GS = World->GetGameState<ATDLobbyGameState>())
		{
			GS->OnPlayerSlotsChanged.RemoveDynamic(this, &UTDWaitingRoomWidget::HandlePlayerSlotsChanged);
		}
	}

	Super::NativeDestruct();
}

// ── 버튼 핸들러 ───────────────────────────────────────────────────────────────

void UTDWaitingRoomWidget::OnStartClicked()
{
	if (ATDLobbyPlayerController* PC = GetLobbyPC())
	{
		PC->Server_RequestStart();
		UE_LOG(LogTemp, Log, TEXT("[WaitingRoom] 게임 시작 요청."));
	}
}

void UTDWaitingRoomWidget::OnLeaveClicked()
{
	// 세션 먼저 파괴
	if (UTDLobbySessionSubsystem* Lobby = GetLobbySession())
		Lobby->DestroySession();

	// 메인 메뉴로 복귀
	if (APlayerController* PC = GetOwningPlayer())
		PC->ClientTravel(MainMenuLevelPath, ETravelType::TRAVEL_Absolute);

	UE_LOG(LogTemp, Log, TEXT("[WaitingRoom] 세션 나가기 → 메인 메뉴 복귀."));
}

// ── 슬롯 변경 콜백 ────────────────────────────────────────────────────────────

void UTDWaitingRoomWidget::HandlePlayerSlotsChanged()
{
	RefreshPlayerList();
}

void UTDWaitingRoomWidget::RefreshPlayerList()
{
	if (ScrollBox_players)
		ScrollBox_players->ClearChildren();

	UWorld* World = GetWorld();
	ATDLobbyGameState* GS = World ? World->GetGameState<ATDLobbyGameState>() : nullptr;
	if (!GS || !PlayerEntryWidgetClass) return;

	for (ATDLobbyPlayerState* PS : GS->PlayerSlots)
	{
		if (!PS) continue;

		UUserWidget* Entry = CreateWidget<UUserWidget>(GetOwningPlayer(), PlayerEntryWidgetClass);
		if (!Entry) continue;

		// BP 자식에서 WBP_PlayerEntry 캐스트 후 이름/준비 상태 초기화
		OnInitPlayerEntry(Entry, PS);

		if (ScrollBox_players)
			ScrollBox_players->AddChild(Entry);
	}

	UE_LOG(LogTemp, Log, TEXT("[WaitingRoom] 플레이어 목록 갱신: %d 명."), GS->PlayerSlots.Num());
}

// ── 유틸 ──────────────────────────────────────────────────────────────────────

UTDLobbySessionSubsystem* UTDWaitingRoomWidget::GetLobbySession() const
{
	if (UGameInstance* GI = GetGameInstance())
		return GI->GetSubsystem<UTDLobbySessionSubsystem>();
	return nullptr;
}

ATDLobbyPlayerController* UTDWaitingRoomWidget::GetLobbyPC() const
{
	return Cast<ATDLobbyPlayerController>(GetOwningPlayer());
}
