#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TDLobbyGameState.generated.h"

class ATDLobbyPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTDOnPlayerSlotsChanged);

/**
 * 로비 전용 GameState.
 * - PlayerSlots: 참가자 PlayerState 배열, 복제.
 * - 서버의 PostLogin/Logout 에서 Add/Remove 후 클라 전체에 자동 동기화.
 */
UCLASS()
class TOWERDEFENCE_API ATDLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

// ── 생명주기 ──────────────────────────────────────────────────────────────────
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

// ── 참가자 슬롯 ───────────────────────────────────────────────────────────────
public:
	/** 서버 전용: 새 참가자 추가 */
	void AddPlayer(ATDLobbyPlayerState* PS);

	/** 서버 전용: 참가자 제거 */
	void RemovePlayer(ATDLobbyPlayerState* PS);

	/** 모든 참가자 준비 완료 여부 */
	UFUNCTION(BlueprintPure, Category = "TD|Lobby")
	bool AreAllPlayersReady() const;

	UPROPERTY(ReplicatedUsing = OnRep_PlayerSlots, BlueprintReadOnly, Category = "TD|Lobby")
	TArray<TObjectPtr<ATDLobbyPlayerState>> PlayerSlots;

	/** 복제 변경 시 위젯에 알리는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "TD|Lobby")
	FTDOnPlayerSlotsChanged OnPlayerSlotsChanged;

private:
	UFUNCTION()
	void OnRep_PlayerSlots();
};
