#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TDLobbyPlayerController.generated.h"

/**
 * 로비 전용 PlayerController.
 * - Server_SetReady: 본인 준비 상태 토글.
 * - Server_RequestStart: 호스트만 게임 시작 요청 (Authority 게이트 검증).
 * - Client_ReturnToMainMenu: 서버 → 클라 복귀 명령 (호스트 종료/강제 퇴장).
 */
UCLASS()
class TOWERDEFENCE_API ATDLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

// ── 생명주기 ──────────────────────────────────────────────────────────────────
protected:
	virtual void BeginPlay() override;

// ── 로비 액션 (Client → Server RPC) ──────────────────────────────────────────
public:
	/**
	 * 클라이언트가 준비 상태를 토글 → 서버에서 PlayerState 갱신 후 복제.
	 * @param bReady true=준비완료, false=준비취소
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "TD|Lobby")
	void Server_SetReady(bool bReady);

	/**
	 * 호스트가 게임 시작 요청.
	 * 검증: 호스트 PC 여부 + 모든 플레이어 준비 완료 확인.
	 * 통과 시 ServerTravel 로 게임플레이 레벨 이동.
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "TD|Lobby")
	void Server_RequestStart();

// ── 서버 → 클라 명령 ─────────────────────────────────────────────────────────
public:
	/**
	 * 서버에서 강제 호출 → 클라이언트가 메인 메뉴로 복귀.
	 * 호스트 종료 / 강제 퇴장 시 사용.
	 * @param Reason 사유 텍스트 (UI 에 표시 가능)
	 */
	UFUNCTION(Client, Reliable, Category = "TD|Lobby")
	void Client_ReturnToMainMenu(const FString& Reason);

// ── 설정 ──────────────────────────────────────────────────────────────────────
public:
	/** 호스트가 StartGame 시 이동할 레벨 경로. BP_LobbyGameMode 에서 설정. */
	UPROPERTY(EditDefaultsOnly, Category = "TD|Lobby")
	FString GameLevelPath = TEXT("/Game/Levels/Levels-01");

	/**
	 * 대기실 위젯 클래스. BP_LobbyPlayerController 의 Details 패널에서 WBP_WaitingRoom 지정.
	 * ListenServer / Client 진입 시 자동 생성됨.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "TD|Lobby|UI")
	TSubclassOf<UUserWidget> WaitingRoomWidgetClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> WaitingRoomWidget;
};
