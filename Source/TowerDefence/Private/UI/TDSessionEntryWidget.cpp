#include "UI/TDSessionEntryWidget.h"
#include "Session/TDLobbySessionSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

// ── 생명주기 ──────────────────────────────────────────────────────────────────

void UTDSessionEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Join)
		Button_Join->OnClicked.AddDynamic(this, &UTDSessionEntryWidget::OnJoinClicked);
}

// ── 공개 API ──────────────────────────────────────────────────────────────────

void UTDSessionEntryWidget::InitWithSessionInfo(const FTDSessionInfo& Info)
{
	CachedSessionIndex = Info.SearchResultIndex;

	// 호스트 이름
	if (Text_HostName)
		Text_HostName->SetText(FText::FromString(Info.OwnerName));

	// 인원 표시: "현재 / 최대" (현재 인원 = 최대 - 남은 자리)
	if (Text_Players)
	{
		const int32 Current = Info.MaxPublicConnections - Info.NumOpenPublicConnections;
		const FString PlayersStr = FString::Printf(TEXT("%d / %d"), Current, Info.MaxPublicConnections);
		Text_Players->SetText(FText::FromString(PlayersStr));
	}

	UE_LOG(LogTemp, Log, TEXT("[SessionEntry] Init — Host: %s, Players: %d/%d, Index: %d"),
		*Info.OwnerName,
		Info.MaxPublicConnections - Info.NumOpenPublicConnections,
		Info.MaxPublicConnections,
		Info.SearchResultIndex);
}

void UTDSessionEntryWidget::SetHostName(const FString& Name)
{
	if (Text_HostName)
		Text_HostName->SetText(FText::FromString(Name));
}

void UTDSessionEntryWidget::SetSessionIndex(int32 Index)
{
	CachedSessionIndex = Index;
}

// ── 버튼 핸들러 ───────────────────────────────────────────────────────────────

void UTDSessionEntryWidget::OnJoinClicked()
{
	if (CachedSessionIndex < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SessionEntry] JoinClicked: SessionIndex 가 설정되지 않았습니다."));
		return;
	}

	if (UTDLobbySessionSubsystem* Lobby = GetLobbySession())
	{
		UE_LOG(LogTemp, Log, TEXT("[SessionEntry] JoinSession 요청 — Index: %d"), CachedSessionIndex);
		Lobby->JoinSession(CachedSessionIndex);
	}
}

// ── 유틸 ──────────────────────────────────────────────────────────────────────

UTDLobbySessionSubsystem* UTDSessionEntryWidget::GetLobbySession() const
{
	if (UGameInstance* GI = GetGameInstance())
		return GI->GetSubsystem<UTDLobbySessionSubsystem>();
	return nullptr;
}
