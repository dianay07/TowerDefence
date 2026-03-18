#include "TDGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "TDMainSaveGame.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

const FString MainSaveSlotName = TEXT("MainSave");
const FName TD_SESSION_NAME = NAME_GameSession;

UTDGameInstance::UTDGameInstance()
{
	SaveSlotIndex = 0;
	SaveSlotNames.Add(0, TEXT("SaveSlotOne"));
	SaveSlotNames.Add(1, TEXT("SaveSlotTwo"));
	SaveSlotNames.Add(2, TEXT("SaveSlotThree"));
}

void UTDGameInstance::Init()
{
	Super::Init();

	LoadMain();

	// OnlineSubsystem 초기화 (bForceLAN은 이 시점에 한 번만 결정)
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();

		// OnlineSubsystemNull은 LAN 전용 → 이후 모든 세션 호출에 LAN 강제
		if (OnlineSubsystem->GetSubsystemName() == FName(TEXT("NULL")))
		{
			bForceLAN = true;
		}

		UE_LOG(LogTemp, Log, TEXT("OnlineSubsystem: %s / ForceLAN: %s"),
			*OnlineSubsystem->GetSubsystemName().ToString(),
			bForceLAN ? TEXT("true") : TEXT("false"));
	}
	else
	{
		// Subsystem 자체가 없으면 LAN으로 폴백
		bForceLAN = true;
		UE_LOG(LogTemp, Warning, TEXT("OnlineSubsystem을 찾을 수 없습니다. LAN 강제."));
	}
}

// ─────────────────────────────────────────────────────────
// 게임 맵 경로 설정
// ─────────────────────────────────────────────────────────

void UTDGameInstance::SetGameMapPath(const FString& MapPath)
{
	GameMapPath = MapPath;
}

// ─────────────────────────────────────────────────────────
// 세션 생성
// ─────────────────────────────────────────────────────────

void UTDGameInstance::CreateSession(int32 NumPublicConnections, bool bIsLAN)
{
	if (bForceLAN) bIsLAN = true;

	if (!OnlineSessionInterface.IsValid())
	{
		OnCreateSessionCompleteDelegate.Broadcast(false);
		return;
	}

	// 이미 존재하는 세션이 있으면 먼저 제거
	FNamedOnlineSession* ExistingSession = OnlineSessionInterface->GetNamedSession(TD_SESSION_NAME);
	if (ExistingSession)
	{
		bPendingCreateAfterDestroy = true;
		PendingNumConnections = NumPublicConnections;
		bPendingIsLAN = bIsLAN;
		DestroySession();
		return;
	}

	// 델리게이트 등록
	CreateSessionCompleteDelegateHandle = OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UTDGameInstance::OnCreateSessionComplete)
	);

	// 세션 설정
	TSharedPtr<FOnlineSessionSettings> Settings = MakeShared<FOnlineSessionSettings>();
	Settings->bIsLANMatch = bIsLAN;
	Settings->NumPublicConnections = NumPublicConnections;
	Settings->bAllowJoinInProgress = true;
	Settings->bAllowInvites = true;
	Settings->bShouldAdvertise = true;
	Settings->bUsesPresence = !bIsLAN;
	Settings->bUseLobbiesIfAvailable = !bIsLAN;

	FString ServerNameToSet = PendingServerName.IsEmpty() ? TEXT("TowerDefence_Host") : PendingServerName;
	Settings->Set(FName(TEXT("SERVER_NAME")), ServerNameToSet, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	if (!OnlineSessionInterface->CreateSession(0, TD_SESSION_NAME, *Settings))
	{
		OnlineSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		OnCreateSessionCompleteDelegate.Broadcast(false);
	}
}

void UTDGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (OnlineSessionInterface.IsValid())
	{
		OnlineSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	UE_LOG(LogTemp, Log, TEXT("세션 생성 %s: %s"), *SessionName.ToString(), bWasSuccessful ? TEXT("성공") : TEXT("실패"));

	OnCreateSessionCompleteDelegate.Broadcast(bWasSuccessful);
}

// ─────────────────────────────────────────────────────────
// 세션 검색
// ─────────────────────────────────────────────────────────

void UTDGameInstance::FindSessions(bool bIsLAN)
{
	if (bForceLAN) bIsLAN = true;

	if (!OnlineSessionInterface.IsValid())
	{
		OnFindSessionsCompleteDelegate.Broadcast(false);
		return;
	}

	FindSessionsCompleteDelegateHandle = OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UTDGameInstance::OnFindSessionsComplete)
	);

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = 20;
	SessionSearch->bIsLanQuery = bIsLAN;
	SessionSearch->TimeoutInSeconds = 10.0f;

	if (!bIsLAN)
	{
		SessionSearch->QuerySettings.Set(FName(TEXT("presence")), true, EOnlineComparisonOp::Equals);
	}

	if (!OnlineSessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		OnlineSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		OnFindSessionsCompleteDelegate.Broadcast(false);
	}
}

void UTDGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (OnlineSessionInterface.IsValid())
	{
		OnlineSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	FoundSessions.Empty();

	if (bWasSuccessful && SessionSearch.IsValid())
	{
		for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)
		{
			FTDSessionInfo Info;
			Info.HostName = Result.Session.OwningUserName.IsEmpty() ? TEXT("Unknown") : Result.Session.OwningUserName;
			Info.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
			Info.CurrentPlayers = Info.MaxPlayers - Result.Session.NumOpenPublicConnections;
			Info.bIsLAN = Result.Session.SessionSettings.bIsLANMatch;

			FString ParsedServerName;
			if (Result.Session.SessionSettings.Get(FName(TEXT("SERVER_NAME")), ParsedServerName))
			{
				Info.ServerName = ParsedServerName;
			}
			else
			{
				Info.ServerName = Info.HostName;
			}

			FoundSessions.Add(Info);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("세션 검색 완료: %d개 발견"), FoundSessions.Num());
	OnFindSessionsCompleteDelegate.Broadcast(bWasSuccessful);
}

// ─────────────────────────────────────────────────────────
// 세션 참가
// ─────────────────────────────────────────────────────────

void UTDGameInstance::JoinFoundSession(int32 Index)
{
	if (!OnlineSessionInterface.IsValid() || !SessionSearch.IsValid())
	{
		OnJoinSessionCompleteDelegate.Broadcast(false);
		return;
	}

	if (!SessionSearch->SearchResults.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("잘못된 세션 인덱스: %d"), Index);
		OnJoinSessionCompleteDelegate.Broadcast(false);
		return;
	}

	JoinSessionCompleteDelegateHandle = OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UTDGameInstance::OnJoinSessionComplete)
	);

	OnlineSessionInterface->JoinSession(0, TD_SESSION_NAME, SessionSearch->SearchResults[Index]);
}

void UTDGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (OnlineSessionInterface.IsValid())
	{
		OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);

	if (bSuccess)
	{
		FString ConnectURL;
		if (OnlineSessionInterface->GetResolvedConnectString(SessionName, ConnectURL))
		{
			APlayerController* PC = GetFirstLocalPlayerController();
			if (PC)
			{
				UE_LOG(LogTemp, Log, TEXT("서버에 접속합니다: %s"), *ConnectURL);
				PC->ClientTravel(ConnectURL, ETravelType::TRAVEL_Absolute);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("연결 URL을 가져오는 데 실패했습니다."));
			bSuccess = false;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("세션 참가 %s"), bSuccess ? TEXT("성공") : TEXT("실패"));
	OnJoinSessionCompleteDelegate.Broadcast(bSuccess);
}

// ─────────────────────────────────────────────────────────
// 세션 제거
// ─────────────────────────────────────────────────────────

void UTDGameInstance::DestroySession()
{
	if (!OnlineSessionInterface.IsValid())
	{
		OnDestroySessionCompleteDelegate.Broadcast(false);
		return;
	}

	DestroySessionCompleteDelegateHandle = OnlineSessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UTDGameInstance::OnDestroySessionComplete)
	);

	if (!OnlineSessionInterface->DestroySession(TD_SESSION_NAME))
	{
		OnlineSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		bPendingCreateAfterDestroy = false;
		OnDestroySessionCompleteDelegate.Broadcast(false);
	}
}

void UTDGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (OnlineSessionInterface.IsValid())
	{
		OnlineSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	if (bPendingCreateAfterDestroy && bWasSuccessful)
	{
		bPendingCreateAfterDestroy = false;
		CreateSession(PendingNumConnections, bPendingIsLAN);
		return;
	}

	bPendingCreateAfterDestroy = false;
	OnDestroySessionCompleteDelegate.Broadcast(bWasSuccessful);
}

// ─────────────────────────────────────────────────────────
// 게임 맵 이동 (호스트 전용)
// ─────────────────────────────────────────────────────────

void UTDGameInstance::ServerTravelToGameMap()
{
	if (GameMapPath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("GameMapPath가 설정되지 않았습니다."));
		return;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		FString TravelURL = GameMapPath + TEXT("?listen");
		World->ServerTravel(TravelURL);
	}
}

// ─────────────────────────────────────────────────────────
// 호스트 편의 함수 (세션 생성 + 자동 ServerTravel)
// ─────────────────────────────────────────────────────────

void UTDGameInstance::HostGame(FString ServerName, int32 MaxPlayers, bool bIsLAN)
{
	PendingServerName = ServerName.IsEmpty() ? TEXT("TowerDefence_Host") : ServerName;
	CreateSession(MaxPlayers, bIsLAN);
}

// ─────────────────────────────────────────────────────────
// 저장 슬롯 API
// ─────────────────────────────────────────────────────────

int32 UTDGameInstance::GetSaveSlot()
{
	return SaveSlotIndex;
}

FString UTDGameInstance::GetSaveSlotName(int32 Index) const
{
	return SaveSlotNames.FindRef(Index);
}

void UTDGameInstance::SetSaveSlot(int32 Index)
{
	SaveSlotIndex = Index;
	SaveMain();
}

void UTDGameInstance::LoadMain()
{
	UTDMainSaveGame* MainSaveGame = Cast<UTDMainSaveGame>(UGameplayStatics::LoadGameFromSlot(MainSaveSlotName, 0));
	if (MainSaveGame)
	{
		SaveSlotIndex = MainSaveGame->SaveSlotIndex;
	}
}

void UTDGameInstance::SaveMain()
{
	UTDMainSaveGame* MainSaveGame = Cast<UTDMainSaveGame>(UGameplayStatics::CreateSaveGameObject(UTDMainSaveGame::StaticClass()));
	if (!MainSaveGame)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create MainSaveGame"));
		return;
	}

	MainSaveGame->SaveSlotIndex = SaveSlotIndex;
	if (!UGameplayStatics::SaveGameToSlot(MainSaveGame, MainSaveSlotName, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save MainSaveGame"));
	}

	LoadMain();
}
