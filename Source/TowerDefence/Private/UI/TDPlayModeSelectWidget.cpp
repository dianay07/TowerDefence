#include "UI/TDPlayModeSelectWidget.h"
#include "Session/TDLevelSessionSubsystem.h"
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
	// TODO: CLAUDE.md §11 LobbySessionSubsystem 완료 후 구현
	// 흐름: LobbySessionSubsystem::CreateSession → 로비 레벨 ServerTravel
	UE_LOG(LogTemp, Log, TEXT("[PlayModeSelect] RequestMultiMode: 멀티 모드 (미구현)."));
}
