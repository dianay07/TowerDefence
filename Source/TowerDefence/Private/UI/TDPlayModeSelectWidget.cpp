#include "UI/TDPlayModeSelectWidget.h"
#include "Session/TDLevelSessionSubsystem.h"
#include "Session/TDLobbySessionSubsystem.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

// ── 생명주기 ──────────────────────────────────────────────────────────────────

void UTDPlayModeSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SoloModeButton)
		SoloModeButton->OnClicked.AddDynamic(this, &UTDPlayModeSelectWidget::OnSoloModeClicked);

	if (MultiModeButton)
		MultiModeButton->OnClicked.AddDynamic(this, &UTDPlayModeSelectWidget::OnMultiModeClicked);

	if (BackButton)
		BackButton->OnClicked.AddDynamic(this, &UTDPlayModeSelectWidget::OnBackClicked);
}

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
	UGameInstance* GI = GetGameInstance();
	UTDLobbySessionSubsystem* Lobby = GI ? GI->GetSubsystem<UTDLobbySessionSubsystem>() : nullptr;
	if (!Lobby)
	{
		UE_LOG(LogTemp, Error, TEXT("[PlayModeSelect] RequestMultiMode: LobbySessionSubsystem 없음."));
		return;
	}

	// StageId 가 있으면 저장, 없어도 세션 브라우저는 열림
	// (게임 시작 시 StageId 검증은 Server_RequestStart 단계에서 처리)
	if (!SelectedStageId.IsNone())
		Lobby->SetMultiStageId(SelectedStageId);

	OnMultiModeRequested(SelectedStageId);

	UE_LOG(LogTemp, Log, TEXT("[PlayModeSelect] RequestMultiMode: StageId=%s"),
		*SelectedStageId.ToString());
}

// ── 버튼 핸들러 ───────────────────────────────────────────────────────────────

void UTDPlayModeSelectWidget::OnSoloModeClicked()
{
	RequestSoloMode();
}

void UTDPlayModeSelectWidget::OnMultiModeClicked()
{
	RequestMultiMode();
}

void UTDPlayModeSelectWidget::OnBackClicked()
{
	OnBackRequested();
}
