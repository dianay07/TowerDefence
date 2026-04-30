#include "UI/TDMultiLobbyWidget.h"
#include "Session/TDLobbySessionSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Blueprint/UserWidget.h"

// ── 생명주기 ──────────────────────────────────────────────────────────────────

void UTDMultiLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CreateRoomButton)
		CreateRoomButton->OnClicked.AddDynamic(this, &UTDMultiLobbyWidget::OnCreateRoomClicked);

	if (RefreshButton)
		RefreshButton->OnClicked.AddDynamic(this, &UTDMultiLobbyWidget::OnRefreshClicked);

	if (BackButton)
		BackButton->OnClicked.AddDynamic(this, &UTDMultiLobbyWidget::OnBackClicked);
}

// ── 버튼 핸들러 ───────────────────────────────────────────────────────────────

void UTDMultiLobbyWidget::OnCreateRoomClicked()
{
	UTDLobbySessionSubsystem* Lobby = GetLobbySession();
	if (!Lobby) return;

	// 중복 바인딩 방지 후 세션 생성
	Lobby->OnSessionCreated.AddUniqueDynamic(this, &UTDMultiLobbyWidget::HandleSessionCreated);
	Lobby->CreateSession(4, true);

	UE_LOG(LogTemp, Log, TEXT("[MultiLobby] CreateSession 요청."));
}

void UTDMultiLobbyWidget::OnRefreshClicked()
{
	if (StatusText)
		StatusText->SetText(FText::FromString(TEXT("검색중..")));

	UTDLobbySessionSubsystem* Lobby = GetLobbySession();
	if (!Lobby) return;

	Lobby->OnSessionsFound.AddUniqueDynamic(this, &UTDMultiLobbyWidget::HandleSessionsFound);
	Lobby->FindSessions(20, true);

	UE_LOG(LogTemp, Log, TEXT("[MultiLobby] FindSessions 요청."));
}

void UTDMultiLobbyWidget::OnBackClicked()
{
	OnBackRequested();
}

// ── 세션 콜백 ─────────────────────────────────────────────────────────────────

void UTDMultiLobbyWidget::HandleSessionCreated(bool bWasSuccessful)
{
	// 콜백 해제 (다음 클릭 시 재등록)
	if (UTDLobbySessionSubsystem* Lobby = GetLobbySession())
		Lobby->OnSessionCreated.RemoveDynamic(this, &UTDMultiLobbyWidget::HandleSessionCreated);

	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("[MultiLobby] 세션 생성 성공 → OnCreateRoomSuccess 발화."));
		OnCreateRoomSuccess(); // BP 에서 OpenLevel(?listen) 처리
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[MultiLobby] 세션 생성 실패."));
		if (StatusText)
			StatusText->SetText(FText::FromString(TEXT("방 생성 실패")));
	}
}

void UTDMultiLobbyWidget::HandleSessionsFound(bool bWasSuccessful)
{
	// 콜백 해제
	UTDLobbySessionSubsystem* Lobby = GetLobbySession();
	if (Lobby)
		Lobby->OnSessionsFound.RemoveDynamic(this, &UTDMultiLobbyWidget::HandleSessionsFound);

	if (!bWasSuccessful)
	{
		if (StatusText)
			StatusText->SetText(FText::FromString(TEXT("검색 실패")));
		return;
	}

	const TArray<FTDSessionInfo> Sessions = Lobby ? Lobby->GetLastFoundSessions() : TArray<FTDSessionInfo>{};

	if (SessionsScrollBox)
		SessionsScrollBox->ClearChildren();

	if (Sessions.IsEmpty())
	{
		if (StatusText)
			StatusText->SetText(FText::FromString(TEXT("검색된 방이 없습니다")));
		return;
	}

	if (StatusText)
		StatusText->SetText(FText::GetEmpty());

	for (const FTDSessionInfo& Info : Sessions)
	{
		if (!SessionEntryWidgetClass) continue;

		UUserWidget* Entry = CreateWidget<UUserWidget>(GetOwningPlayer(), SessionEntryWidgetClass);
		if (!Entry) continue;

		// BP 자식에서 WBP_SessionEntry 캐스트 후 SetSessionIndex / SetHostName 호출
		OnInitSessionEntry(Entry, Info);

		if (SessionsScrollBox)
			SessionsScrollBox->AddChild(Entry);
	}

	UE_LOG(LogTemp, Log, TEXT("[MultiLobby] 세션 목록 갱신: %d 건."), Sessions.Num());
}

// ── 유틸 ──────────────────────────────────────────────────────────────────────

UTDLobbySessionSubsystem* UTDMultiLobbyWidget::GetLobbySession() const
{
	if (UGameInstance* GI = GetGameInstance())
		return GI->GetSubsystem<UTDLobbySessionSubsystem>();
	return nullptr;
}
