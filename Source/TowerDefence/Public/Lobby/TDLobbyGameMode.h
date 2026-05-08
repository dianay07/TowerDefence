#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TDLobbyGameMode.generated.h"

/**
 * 로비 전용 GameMode (서버 전용 실행).
 *
 * 담당:
 *   - PostLogin: 새 참가자를 LobbyGameState::PlayerSlots 에 추가.
 *   - Logout: 퇴장 참가자를 슬롯에서 제거. 호스트 퇴장 시 모든 클라에 복귀 명령.
 *   - 기본 클래스로 ATDLobbyPlayerController / ATDLobbyGameState / ATDLobbyPlayerState 지정.
 *
 * BP 자식 (BP_LobbyGameMode) 에서:
 *   - ATDLobbyPlayerController::GameLevelPath 기본값 설정 가능.
 */
UCLASS()
class TOWERDEFENCE_API ATDLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

// ── 설정 ──────────────────────────────────────────────────────────────────────
public:
	/** 세션 최대 인원 (Listen Server 로 시작될 때 CreateSession 에 사용) */
	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	int32 MaxPlayers = 4;

// ── 생명주기 ──────────────────────────────────────────────────────────────────
public:
	ATDLobbyGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

// ── 호스트 종료 처리 ─────────────────────────────────────────────────────────
private:
	/** 호스트가 나갔을 때 모든 클라에 복귀 명령 전송. */
	void NotifyAllClientsHostLeft();
};
