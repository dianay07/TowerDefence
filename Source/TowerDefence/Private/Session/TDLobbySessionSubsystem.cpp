#include "Session/TDLobbySessionSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"

namespace
{
	// 세션 이름 — 프로젝트 전체에서 단일 세션만 사용
	static const FName TDSessionName = NAME_GameSession;
}

UTDLobbySessionSubsystem::UTDLobbySessionSubsystem()
{
}

// ── 생명주기 ──────────────────────────────────────────────────────────────────

void UTDLobbySessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
	{
		SessionInterface = OSS->GetSessionInterface();
	}

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbySession] OnlineSubsystem 의 SessionInterface 를 가져오지 못했습니다."));
	}
}

// ── 세션 관리 API ─────────────────────────────────────────────────────────────

void UTDLobbySessionSubsystem::CreateSession(int32 NumPublicConnections, bool bIsLAN)
{
	if (!SessionInterface.IsValid()) return;

	// 잔존 세션 있으면 충돌 → 먼저 제거
	if (SessionInterface->GetNamedSession(TDSessionName))
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbySession] CreateSession: 기존 세션 발견 → DestroySession 후 재시도."));
		DestroySession();
		return;
	}

	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UTDLobbySessionSubsystem::OnCreateSessionComplete));

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections  = NumPublicConnections;
	Settings.bIsLANMatch           = bIsLAN;
	Settings.bAllowJoinInProgress  = false;
	Settings.bShouldAdvertise      = true;
	Settings.bUsesPresence         = true;		// Null 서브시스템 검색에 필요
	Settings.bUseLobbiesIfAvailable = false;	// Null 은 미지원

	const ULocalPlayer* LP = GetGameInstance()->GetFirstGamePlayer();
	if (!LP)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbySession] CreateSession: LocalPlayer 없음."));
		return;
	}

	SessionInterface->CreateSession(*LP->GetPreferredUniqueNetId(), TDSessionName, Settings);
}

void UTDLobbySessionSubsystem::FindSessions(int32 MaxResults, bool bIsLAN)
{
	if (!SessionInterface.IsValid()) return;

	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UTDLobbySessionSubsystem::OnFindSessionsComplete));

	LastSessionSearch = MakeShared<FOnlineSessionSearch>();
	LastSessionSearch->MaxSearchResults = MaxResults;
	LastSessionSearch->bIsLanQuery      = bIsLAN;
	// SEARCH_PRESENCE = TEXT("presence") — 헤더 의존 없이 리터럴로 직접 지정
	LastSessionSearch->QuerySettings.Set(FName(TEXT("presence")), true, EOnlineComparisonOp::Equals);

	const ULocalPlayer* LP = GetGameInstance()->GetFirstGamePlayer();
	if (!LP) return;

	SessionInterface->FindSessions(*LP->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef());
}

void UTDLobbySessionSubsystem::JoinSession(int32 SearchResultIndex)
{
	if (!SessionInterface.IsValid() || !LastSessionSearch.IsValid()) return;

	if (!LastSessionSearch->SearchResults.IsValidIndex(SearchResultIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbySession] JoinSession: 잘못된 인덱스 %d"), SearchResultIndex);
		OnSessionJoined.Broadcast(false);
		return;
	}

	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UTDLobbySessionSubsystem::OnJoinSessionComplete));

	const ULocalPlayer* LP = GetGameInstance()->GetFirstGamePlayer();
	if (!LP) return;

	SessionInterface->JoinSession(
		*LP->GetPreferredUniqueNetId(),
		TDSessionName,
		LastSessionSearch->SearchResults[SearchResultIndex]);
}

void UTDLobbySessionSubsystem::DestroySession()
{
	if (!SessionInterface.IsValid()) return;

	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UTDLobbySessionSubsystem::OnDestroySessionComplete));

	SessionInterface->DestroySession(TDSessionName);
}

bool UTDLobbySessionSubsystem::HasActiveSession() const
{
	if (!SessionInterface.IsValid()) return false;
	return SessionInterface->GetNamedSession(TDSessionName) != nullptr;
}

// ── 내부 구현 — OnlineSubsystem 콜백 ─────────────────────────────────────────

void UTDLobbySessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

	UE_LOG(LogTemp, Log, TEXT("[LobbySession] CreateSession 완료: %s"), bWasSuccessful ? TEXT("성공") : TEXT("실패"));
	OnSessionCreated.Broadcast(bWasSuccessful);
}

void UTDLobbySessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

	// FOnlineSessionSearchResult → FTDSessionInfo 변환 후 내부 캐시에 보관
	// BP 는 OnSessionsFound 콜백 후 GetLastFoundSessions() 로 결과를 조회한다.
	LastFoundSessions.Empty();
	if (bWasSuccessful && LastSessionSearch.IsValid())
	{
		for (int32 i = 0; i < LastSessionSearch->SearchResults.Num(); ++i)
		{
			const FOnlineSessionSearchResult& R = LastSessionSearch->SearchResults[i];

			FTDSessionInfo Info;
			Info.OwnerName                = R.Session.OwningUserName;
			Info.NumOpenPublicConnections = R.Session.NumOpenPublicConnections;
			Info.MaxPublicConnections     = R.Session.SessionSettings.NumPublicConnections;
			Info.PingInMs                 = R.PingInMs;
			Info.SearchResultIndex        = i;
			LastFoundSessions.Add(Info);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[LobbySession] FindSessions 완료: %d 건"), LastFoundSessions.Num());
	OnSessionsFound.Broadcast(bWasSuccessful);
}

void UTDLobbySessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

	const bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);

	if (bSuccess)
	{
		// 접속 URL 을 얻어서 ClientTravel 실행
		FString ConnectInfo;
		if (SessionInterface->GetResolvedConnectString(TDSessionName, ConnectInfo))
		{
			if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
			{
				PC->ClientTravel(ConnectInfo, ETravelType::TRAVEL_Absolute);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[LobbySession] GetResolvedConnectString 실패."));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[LobbySession] JoinSession 완료: %s"), bSuccess ? TEXT("성공") : TEXT("실패"));
	OnSessionJoined.Broadcast(bSuccess);
}

void UTDLobbySessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);

	UE_LOG(LogTemp, Log, TEXT("[LobbySession] DestroySession 완료: %s"), bWasSuccessful ? TEXT("성공") : TEXT("실패"));
	OnSessionDestroyed.Broadcast(bWasSuccessful);
}
