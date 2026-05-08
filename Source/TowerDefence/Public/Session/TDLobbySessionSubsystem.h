#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "TDLobbySessionSubsystem.generated.h"

/**
 * BP / 위젯에 노출하는 경량 세션 정보 구조체.
 * FOnlineSessionSearchResult 는 USTRUCT 가 아니라 직접 UPROPERTY 불가 → 래핑.
 */
USTRUCT(BlueprintType)
struct FTDSessionInfo
{
	GENERATED_BODY()

	/** 세션 호스트 이름 */
	UPROPERTY(BlueprintReadOnly, Category = "TD|Lobby")
	FString OwnerName;

	/** 접속 가능한 빈 슬롯 수 */
	UPROPERTY(BlueprintReadOnly, Category = "TD|Lobby")
	int32 NumOpenPublicConnections = 0;

	/** 최대 인원 */
	UPROPERTY(BlueprintReadOnly, Category = "TD|Lobby")
	int32 MaxPublicConnections = 0;

	/** 핑 (ms). Null 서브시스템은 항상 0. */
	UPROPERTY(BlueprintReadOnly, Category = "TD|Lobby")
	int32 PingInMs = 0;

	/** JoinSession 호출 시 사용할 내부 인덱스 (Subsystem 의 LastSearchResults) */
	UPROPERTY(BlueprintReadOnly, Category = "TD|Lobby")
	int32 SearchResultIndex = INDEX_NONE;
};

// ── 델리게이트 선언 ────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (FTDOnSessionCreated,   bool,                    bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (FTDOnSessionsFound,    bool,                    bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (FTDOnSessionJoined,    bool,                    bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (FTDOnSessionDestroyed, bool,                    bWasSuccessful);

/**
 * 멀티 세션 단일 허브 (GameInstanceSubsystem).
 *
 * 역할:
 *   - IOnlineSession API (Create / Find / Join / Destroy) 래핑.
 *   - 위젯 / PlayerController 가 IOnlineSession 직접 호출 금지 (CLAUDE.md §11-2 규칙).
 *   - 각 작업 완료 시 Dynamic Multicast 브로드캐스트 → 위젯이 구독.
 *
 * 이름 구분:
 *   - 이 클래스 = 멀티 세션 관리 (OnlineSubsystem 기반)
 *   - UTDLevelSessionSubsystem = 레벨/스테이지 전환 담당 (별개 책임)
 */
UCLASS()
class TOWERDEFENCE_API UTDLobbySessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UTDLobbySessionSubsystem();

// ── 생명주기 ──────────────────────────────────────────────────────────────────
protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

// ── 세션 관리 API ─────────────────────────────────────────────────────────────
public:
	/**
	 * 세션을 생성한다. 호스트가 방을 열 때 호출.
	 * 완료 시 OnSessionCreated 브로드캐스트.
	 * @param NumPublicConnections 최대 참가 인원 (호스트 포함)
	 * @param bIsLAN LAN 전용 세션 여부 (OnlineSubsystemNull 은 true 필수)
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|LobbySession")
	void CreateSession(int32 NumPublicConnections, bool bIsLAN);

	/**
	 * 열린 세션 목록을 검색한다.
	 * 완료 시 OnSessionsFound 브로드캐스트.
	 * @param MaxResults 최대 결과 수
	 * @param bIsLAN LAN 검색 여부 (OnlineSubsystemNull 은 true 필수)
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|LobbySession")
	void FindSessions(int32 MaxResults, bool bIsLAN);

	/**
	 * 세션에 참가한다.
	 * 완료 시 OnSessionJoined 브로드캐스트. 성공 시 RequestingPlayer 의 PC 로 ClientTravel.
	 * @param SearchResultIndex FindSessions 결과의 인덱스 (FTDSessionInfo::SearchResultIndex)
	 * @param RequestingPlayer Join 을 요청한 로컬 플레이어. nullptr 이면 첫 번째 플레이어 사용.
	 *        PIE 다창(Run Under One Process) 환경에서 Window 2 가 Join 할 때 반드시 지정해야 함.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|LobbySession")
	void JoinSession(int32 SearchResultIndex, APlayerController* RequestingPlayer = nullptr);

	/**
	 * 현재 세션을 종료한다.
	 * 호스트: 모든 클라 강제 퇴장 후 호출. 클라이언트: 자체 세션만 정리.
	 * 완료 시 OnSessionDestroyed 브로드캐스트.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|LobbySession")
	void DestroySession();

	/** 세션이 활성 상태인지 확인. */
	UFUNCTION(BlueprintPure, Category = "TD|LobbySession")
	bool HasActiveSession() const;

	/**
	 * FindSessions 완료 후 결과 목록을 반환.
	 * OnSessionsFound 콜백(bWasSuccessful=true) 내에서 호출해 방 목록을 가져온다.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|LobbySession")
	TArray<FTDSessionInfo> GetLastFoundSessions() const { return LastFoundSessions; }

	// ── 멀티 스테이지 선택 ────────────────────────────────────────────────────────

	/**
	 * 호스트가 플레이할 스테이지 ID 를 저장한다.
	 * RequestMultiMode() 에서 세션 생성 전에 호출 → Server_RequestStart 에서 맵 경로 조회에 사용.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|LobbySession")
	void SetMultiStageId(FName StageId) { MultiStageId = StageId; }

	/** 현재 설정된 멀티 스테이지 ID. */
	UFUNCTION(BlueprintPure, Category = "TD|LobbySession")
	FName GetMultiStageId() const { return MultiStageId; }

// ── 델리게이트 (위젯이 구독) ──────────────────────────────────────────────────
public:
	UPROPERTY(BlueprintAssignable, Category = "TD|LobbySession")
	FTDOnSessionCreated OnSessionCreated;

	UPROPERTY(BlueprintAssignable, Category = "TD|LobbySession")
	FTDOnSessionsFound OnSessionsFound;

	UPROPERTY(BlueprintAssignable, Category = "TD|LobbySession")
	FTDOnSessionJoined OnSessionJoined;

	UPROPERTY(BlueprintAssignable, Category = "TD|LobbySession")
	FTDOnSessionDestroyed OnSessionDestroyed;

// ── 내부 구현 ─────────────────────────────────────────────────────────────────
private:
	/**
	 * 현재 World 컨텍스트에 맞는 SessionInterface 반환 (지연 초기화).
	 *
	 * 이유: Initialize() 시점에는 World 가 없어 IOnlineSubsystem::Get() 이
	 *       PIE 의 올바른 컨텍스트(:Context_N) 대신 기본 "Null" 인스턴스를 반환,
	 *       결과적으로 GetPortFromNetDriver 가 NetDriver 를 못 찾고 포트 0 을 등록.
	 *       Online::GetSubsystem(World) 를 쓰면 올바른 Context 인스턴스를 획득.
	 */
	IOnlineSessionPtr GetSessionInterface();

	IOnlineSessionPtr SessionInterface;          // 캐시 (World 확보 후 1회 세팅)
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	// OnlineSubsystem 콜백 핸들러
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	// CreateSession 재시도: Destroy 완료 후 원래 파라미터로 재생성
	bool bPendingCreateAfterDestroy = false;
	int32 PendingNumPublicConnections = 4;
	bool  bPendingIsLAN = true;

	// JoinSession 재시도: 잔존 세션 Destroy 후 Join 재시도
	bool bPendingJoinAfterDestroy = false;
	int32 PendingJoinIndex = INDEX_NONE;

	// JoinSession: 어느 플레이어가 Join 을 요청했는지 기억 (ClientTravel 대상)
	TWeakObjectPtr<APlayerController> PendingJoinPC;

	/** 호스트가 선택한 스테이지 ID. Server_RequestStart 가 맵 경로 조회에 사용. */
	FName MultiStageId;

	/** FindSessions 결과 캐시. GetLastFoundSessions() 로 BP 에 노출. */
	TArray<FTDSessionInfo> LastFoundSessions;

	// 델리게이트 핸들 (Unbind 용)
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
};
