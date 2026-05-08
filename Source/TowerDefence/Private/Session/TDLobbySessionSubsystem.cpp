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
	// SessionInterface 는 GetSessionInterface() 에서 지연 초기화.
	// Initialize 시점엔 World 가 없어 올바른 OSS 컨텍스트를 얻을 수 없기 때문.
}

IOnlineSessionPtr UTDLobbySessionSubsystem::GetSessionInterface()
{
	// 이미 캐시돼 있으면 그대로 반환
	if (SessionInterface.IsValid()) return SessionInterface;

	// World 컨텍스트를 이용해 PIE 전용 OSS 인스턴스 획득.
	// Online::GetSubsystem(World) 는 ":Context_N" 같은 PIE 전용 인스턴스를 반환하므로
	// GetPortFromNetDriver 가 실제 NetDriver 포트(7777 등)를 올바르게 읽음.
	UGameInstance* GI = GetGameInstance();
	UWorld* World     = GI ? GI->GetWorld() : nullptr;

	IOnlineSubsystem* OSS = World ? Online::GetSubsystem(World) : IOnlineSubsystem::Get();
	if (!OSS) OSS = IOnlineSubsystem::Get();  // 폴백

	if (OSS)
	{
		SessionInterface = OSS->GetSessionInterface();
		UE_LOG(LogTemp, Log, TEXT("[LobbySession] OSS 획득: %s (context=%s)"),
			*OSS->GetSubsystemName().ToString(), *OSS->GetInstanceName().ToString());
	}

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbySession] SessionInterface 초기화 실패."));
	}
	return SessionInterface;
}

// ── 세션 관리 API ─────────────────────────────────────────────────────────────

void UTDLobbySessionSubsystem::CreateSession(int32 NumPublicConnections, bool bIsLAN)
{
	IOnlineSessionPtr SI = GetSessionInterface();
	if (!SI.IsValid()) return;

	// 잔존 세션 있으면 충돌 → 먼저 제거 후 재시도 (파라미터 보관)
	if (SI->GetNamedSession(TDSessionName))
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbySession] CreateSession: 기존 세션 발견 → DestroySession 후 재시도."));
		bPendingCreateAfterDestroy   = true;
		PendingNumPublicConnections  = NumPublicConnections;
		bPendingIsLAN                = bIsLAN;
		DestroySession();
		return;
	}

	CreateSessionCompleteDelegateHandle = SI->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UTDLobbySessionSubsystem::OnCreateSessionComplete));

	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections   = NumPublicConnections;
	Settings.bIsLANMatch            = bIsLAN;
	Settings.bAllowJoinInProgress   = false;
	Settings.bShouldAdvertise       = true;
	Settings.bUsesPresence          = false;	// Null OSS 는 presence 미지원. true 이면 LAN 검색과 충돌.
	Settings.bUseLobbiesIfAvailable = false;	// Null 은 미지원

	const ULocalPlayer* LP = GetGameInstance()->GetFirstGamePlayer();
	if (!LP)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbySession] CreateSession: LocalPlayer 없음."));
		return;
	}

	SI->CreateSession(*LP->GetPreferredUniqueNetId(), TDSessionName, Settings);
}

void UTDLobbySessionSubsystem::FindSessions(int32 MaxResults, bool bIsLAN)
{
	IOnlineSessionPtr SI = GetSessionInterface();
	if (!SI.IsValid()) return;

	FindSessionsCompleteDelegateHandle = SI->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UTDLobbySessionSubsystem::OnFindSessionsComplete));

	LastSessionSearch = MakeShared<FOnlineSessionSearch>();
	LastSessionSearch->MaxSearchResults = MaxResults;
	LastSessionSearch->bIsLanQuery      = bIsLAN;
	// OnlineSubsystemNull 은 presence 필터를 지원하지 않으므로 제거.
	// LAN 검색은 bIsLanQuery = true 만으로 충분.

	const ULocalPlayer* LP = GetGameInstance()->GetFirstGamePlayer();
	if (!LP) return;

	SI->FindSessions(*LP->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef());
}

void UTDLobbySessionSubsystem::JoinSession(int32 SearchResultIndex, APlayerController* RequestingPlayer)
{
	IOnlineSessionPtr SI = GetSessionInterface();
	if (!SI.IsValid() || !LastSessionSearch.IsValid()) return;

	if (!LastSessionSearch->SearchResults.IsValidIndex(SearchResultIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbySession] JoinSession: 잘못된 인덱스 %d"), SearchResultIndex);
		OnSessionJoined.Broadcast(false);
		return;
	}

	// ClientTravel 대상 PC 보관 (PIE 다창에서 Window 2 가 Join 하면 Window 2의 PC 를 써야 함)
	if (RequestingPlayer)
	{
		PendingJoinPC = RequestingPlayer;
	}
	else
	{
		PendingJoinPC = GetGameInstance()->GetFirstLocalPlayerController();
	}

	// 잔존 세션 있으면 먼저 정리 후 재시도 (이전 Join 실패로 세션 항목이 남아있는 경우)
	if (SI->GetNamedSession(TDSessionName))
	{
		UE_LOG(LogTemp, Log, TEXT("[LobbySession] JoinSession: 잔존 세션 발견 → DestroySession 후 재시도."));
		bPendingJoinAfterDestroy = true;
		PendingJoinIndex         = SearchResultIndex;
		DestroySession();
		return;
	}

	JoinSessionCompleteDelegateHandle = SI->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UTDLobbySessionSubsystem::OnJoinSessionComplete));

	const ULocalPlayer* LP = PendingJoinPC.IsValid()
		? PendingJoinPC->GetLocalPlayer()
		: GetGameInstance()->GetFirstGamePlayer();

	if (!LP)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbySession] JoinSession: LocalPlayer 없음."));
		return;
	}

	SI->JoinSession(
		*LP->GetPreferredUniqueNetId(),
		TDSessionName,
		LastSessionSearch->SearchResults[SearchResultIndex]);
}

void UTDLobbySessionSubsystem::DestroySession()
{
	IOnlineSessionPtr SI = GetSessionInterface();
	if (!SI.IsValid()) return;

	DestroySessionCompleteDelegateHandle = SI->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UTDLobbySessionSubsystem::OnDestroySessionComplete));

	SI->DestroySession(TDSessionName);
}

bool UTDLobbySessionSubsystem::HasActiveSession() const
{
	if (!SessionInterface.IsValid()) return false;
	return SessionInterface->GetNamedSession(TDSessionName) != nullptr;
}

// ── 내부 구현 — OnlineSubsystem 콜백 ─────────────────────────────────────────

void UTDLobbySessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SI = GetSessionInterface();
	if (SI.IsValid()) SI->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

	UE_LOG(LogTemp, Log, TEXT("[LobbySession] CreateSession 완료: %s"), bWasSuccessful ? TEXT("성공") : TEXT("실패"));
	OnSessionCreated.Broadcast(bWasSuccessful);
}

void UTDLobbySessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	IOnlineSessionPtr SI = GetSessionInterface();
	if (SI.IsValid()) SI->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

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

	UE_LOG(LogTemp, Log, TEXT("[LobbySession] FindSessions 완료 — bSuccess=%s, RawResults=%d, Converted=%d"),
		bWasSuccessful ? TEXT("true") : TEXT("false"),
		(LastSessionSearch.IsValid() ? LastSessionSearch->SearchResults.Num() : -1),
		LastFoundSessions.Num());
	OnSessionsFound.Broadcast(bWasSuccessful);
}

void UTDLobbySessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr SI = GetSessionInterface();
	if (SI.IsValid()) SI->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

	const bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);

	// 실패 코드 상세 로그 — 진단용
	if (!bSuccess)
	{
		FString ResultStr;
		switch (Result)
		{
			case EOnJoinSessionCompleteResult::SessionIsFull:           ResultStr = TEXT("SessionIsFull");           break;
			case EOnJoinSessionCompleteResult::SessionDoesNotExist:     ResultStr = TEXT("SessionDoesNotExist");     break;
			case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress: ResultStr = TEXT("CouldNotRetrieveAddress"); break;
			case EOnJoinSessionCompleteResult::AlreadyInSession:        ResultStr = TEXT("AlreadyInSession");        break;
			case EOnJoinSessionCompleteResult::UnknownError:            ResultStr = TEXT("UnknownError");            break;
			default:                                                    ResultStr = TEXT("?");                       break;
		}
		UE_LOG(LogTemp, Warning, TEXT("[LobbySession] JoinSession 실패 — Result=%s"), *ResultStr);
	}

	if (bSuccess)
	{
		FString ConnectInfo;
		const bool bResolved = SI.IsValid() && SI->GetResolvedConnectString(TDSessionName, ConnectInfo);

		if (bResolved)
		{
			APlayerController* PC = PendingJoinPC.IsValid()
				? PendingJoinPC.Get()
				: GetGameInstance()->GetFirstLocalPlayerController();

			UE_LOG(LogTemp, Log, TEXT("[LobbySession] ConnectInfo=%s, PC=%s"),
				*ConnectInfo,
				PC ? *PC->GetName() : TEXT("nullptr"));

			if (PC)
			{
				UE_LOG(LogTemp, Log, TEXT("[LobbySession] ClientTravel → %s"), *ConnectInfo);
				PC->ClientTravel(ConnectInfo, ETravelType::TRAVEL_Absolute);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[LobbySession] PC 없음 → ClientTravel 불가."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[LobbySession] GetResolvedConnectString 실패. 세션의 호스트 IP/포트 조회 불가."));
		}
	}

	PendingJoinPC = nullptr;  // 참조 해제
	UE_LOG(LogTemp, Log, TEXT("[LobbySession] JoinSession 완료: %s"), bSuccess ? TEXT("성공") : TEXT("실패"));
	OnSessionJoined.Broadcast(bSuccess);
}

void UTDLobbySessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SI = GetSessionInterface();
	if (SI.IsValid()) SI->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);

	UE_LOG(LogTemp, Log, TEXT("[LobbySession] DestroySession 완료: %s"), bWasSuccessful ? TEXT("성공") : TEXT("실패"));
	OnSessionDestroyed.Broadcast(bWasSuccessful);

	// CreateSession 호출 중 잔존 세션이 발견돼 Destroy 한 경우 → 재시도
	if (bPendingCreateAfterDestroy)
	{
		bPendingCreateAfterDestroy = false;
		UE_LOG(LogTemp, Log, TEXT("[LobbySession] 재시도: CreateSession(%d, LAN=%s)"),
			PendingNumPublicConnections, bPendingIsLAN ? TEXT("true") : TEXT("false"));
		CreateSession(PendingNumPublicConnections, bPendingIsLAN);
	}
	// JoinSession 호출 중 잔존 세션이 발견돼 Destroy 한 경우 → 재시도
	else if (bPendingJoinAfterDestroy)
	{
		bPendingJoinAfterDestroy = false;
		UE_LOG(LogTemp, Log, TEXT("[LobbySession] 재시도: JoinSession(Index=%d)"), PendingJoinIndex);
		JoinSession(PendingJoinIndex, PendingJoinPC.Get());
	}
}
