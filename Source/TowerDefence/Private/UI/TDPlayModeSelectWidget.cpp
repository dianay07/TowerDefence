#include "UI/TDPlayModeSelectWidget.h"
#include "Session/TDLevelSessionSubsystem.h"
#include "Session/TDLobbySessionSubsystem.h"
#include "Kismet/GameplayStatics.h"

// ── 공개 API ──────────────────────────────────────────────────────────────────

void UTDPlayModeSelectWidget::InitWithStage(FName InStageId)
{
	if (InStageId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayModeSelect] InitWithStage: StageId 가 None 입니다."));
		return;
	}

	SelectedStageId = InStageId;
	OnStageSet(SelectedStageId);

	UE_LOG(LogTemp, Log, TEXT("[PlayModeSelect] 스테이지 선택: %s"), *SelectedStageId.ToString());
}

void UTDPlayModeSelectWidget::RequestSoloMode()
{
	if (SelectedStageId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayModeSelect] RequestSoloMode: 스테이지가 선택되지 않았습니다."));
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UTDLevelSessionSubsystem* LevelSession = GI->GetSubsystem<UTDLevelSessionSubsystem>();
	if (!LevelSession)
	{
		UE_LOG(LogTemp, Error, TEXT("[PlayModeSelect] RequestSoloMode: LevelSessionSubsystem 없음."));
		return;
	}

	const bool bSuccess = LevelSession->RequestLoadStage(SelectedStageId);
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("[PlayModeSelect] RequestSoloMode: StageId '%s' 를 DT_Stages 에서 찾지 못했습니다."),
			*SelectedStageId.ToString());
	}
}

void UTDPlayModeSelectWidget::RequestMultiMode()
{
	if (SelectedStageId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayModeSelect] RequestMultiMode: 스테이지가 선택되지 않았습니다."));
		return;
	}

	UGameInstance* GI = GetGameInstance();
	UTDLobbySessionSubsystem* Lobby = GI ? GI->GetSubsystem<UTDLobbySessionSubsystem>() : nullptr;
	if (!Lobby)
	{
		UE_LOG(LogTemp, Error, TEXT("[PlayModeSelect] RequestMultiMode: LobbySessionSubsystem 없음."));
		return;
	}

	// 선택된 스테이지를 저장 → 로비에서 Server_RequestStart 시 맵 경로 조회에 사용
	Lobby->SetMultiStageId(SelectedStageId);

	// BP 자식에서 Host / Join 선택 UI 를 표시하도록 이벤트 발화
	OnMultiModeRequested(SelectedStageId);

	UE_LOG(LogTemp, Log, TEXT("[PlayModeSelect] RequestMultiMode: 스테이지 '%s' 멀티 모드 요청."),
		*SelectedStageId.ToString());
}
