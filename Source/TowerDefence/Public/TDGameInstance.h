#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "TDGameInstance.generated.h"

// ─────────────────────────────────────────────
// Blueprint에서 사용할 세션 정보 래퍼 구조체
// ─────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FTDSessionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString HostName;

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	bool bIsLAN = false;
};

// ─────────────────────────────────────────────
// Blueprint용 멀티캐스트 델리게이트
// ─────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnFindSessionsComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnJoinSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnDestroySessionComplete, bool, bWasSuccessful);


UCLASS()
class TOWERDEFENCE_API UTDGameInstance : public UGameInstance
{
	GENERATED_BODY()

	// ─────────────────────────────────────────
	// 저장 슬롯
	// ─────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	int32 SaveSlotIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TMap<int32, FString> SaveSlotNames;

	// ─────────────────────────────────────────
	// 온라인 세션 내부 데이터
	// ─────────────────────────────────────────
	IOnlineSessionPtr OnlineSessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	// 내부 콜백 델리게이트 핸들
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;

	// 내부 콜백 함수
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	// 세션 생성 후 이동할 맵 (DestroySession 후 재생성 시 재사용)
	bool bPendingCreateAfterDestroy = false;
	int32 PendingNumConnections = 2;
	bool bPendingIsLAN = false;

public:
	UTDGameInstance();

	virtual void Init() override;

	// ─────────────────────────────────────────
	// Blueprint 노출 델리게이트
	// ─────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category = "Session")
	FTDOnCreateSessionComplete OnCreateSessionCompleteDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FTDOnFindSessionsComplete OnFindSessionsCompleteDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FTDOnJoinSessionComplete OnJoinSessionCompleteDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FTDOnDestroySessionComplete OnDestroySessionCompleteDelegate;

	// 검색된 세션 목록 (Blueprint에서 읽기 가능)
	UPROPERTY(BlueprintReadOnly, Category = "Session")
	TArray<FTDSessionInfo> FoundSessions;

	// 선택한 게임 맵 경로 (예: "/Game/Levels/01/Levels-01")
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Session")
	FString GameMapPath;

	// ─────────────────────────────────────────
	// 세션 API
	// ─────────────────────────────────────────

	/** 레벨 선택 시 이동할 게임 맵 경로를 설정 */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void SetGameMapPath(const FString& MapPath);

	/** 세션 생성 (호스트). NumPublicConnections: 최대 플레이어 수, bIsLAN: LAN 세션 여부 */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void CreateSession(int32 NumPublicConnections = 2, bool bIsLAN = false);

	/** 세션 검색 */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void FindSessions(bool bIsLAN = false);

	/** 검색된 세션에 참가 (FoundSessions 배열의 Index) */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void JoinFoundSession(int32 Index);

	/** 현재 세션 종료 */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void DestroySession();

	/** 세션 생성 후 호스트가 게임 맵으로 이동 (서버 트래블) */
	UFUNCTION(BlueprintCallable, Category = "Session")
	void ServerTravelToGameMap();

	// ─────────────────────────────────────────
	// 저장 슬롯 API
	// ─────────────────────────────────────────
	virtual void LoadMain();
	virtual void SaveMain();

	UFUNCTION(BlueprintPure)
	virtual int32 GetSaveSlot();

	UFUNCTION(BlueprintCallable)
	virtual void SetSaveSlot(int32 Index);

	UFUNCTION(BlueprintPure)
	virtual FString GetSaveSlotName(int32 Index) const;
};
