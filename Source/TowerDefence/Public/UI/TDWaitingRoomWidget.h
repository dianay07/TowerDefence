#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TDWaitingRoomWidget.generated.h"

class UButton;
class UTextBlock;
class UScrollBox;
class ATDLobbyPlayerState;
class ATDLobbyGameState;
class UTDLobbySessionSubsystem;
class ATDLobbyPlayerController;

/**
 * 대기실 위젯 C++ 베이스 (WBP_WaitingRoom 의 부모).
 *
 * 책임:
 *   - Construct  : OnPlayerSlotsChanged 구독 + 초기 목록 갱신 + 호스트 여부로 StartButton 가시성 제어
 *   - Refresh    : ScrollBox_players 를 PlayerSlots 기준으로 다시 채움
 *   - Start      : PC→Server_RequestStart (호스트 전용)
 *   - Leave      : DestroySession + ClientTravel 메인 메뉴
 *   - MapName    : LobbySubsystem 의 MultiStageId 로 Text_MapName 자동 설정
 *
 * BP 자식 (WBP_WaitingRoom) 이 구현할 것:
 *   - OnInitPlayerEntry : WBP_PlayerEntry 로 캐스트 후 이름/준비 상태 표시
 */
UCLASS(Abstract)
class TOWERDEFENCE_API UTDWaitingRoomWidget : public UUserWidget
{
	GENERATED_BODY()

// ── 생명주기 ──────────────────────────────────────────────────────────────────
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

// ── UI 바인딩 (BP 위젯 변수명과 정확히 일치) ──────────────────────────────────
protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_MapName;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> ScrollBox_players;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> StartButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> LeaveButton;

// ── 설정 ──────────────────────────────────────────────────────────────────────
public:
	/**
	 * 참가자 한 줄 위젯 클래스. BP_WaitingRoom Details 에서 WBP_PlayerEntry 지정.
	 * RefreshPlayerList 호출 시 ScrollBox_players 에 자동 생성됨.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "TD|WaitingRoom")
	TSubclassOf<UUserWidget> PlayerEntryWidgetClass;

	/** Leave 시 이동할 메인 메뉴 레벨 경로. */
	UPROPERTY(EditDefaultsOnly, Category = "TD|WaitingRoom")
	FString MainMenuLevelPath = TEXT("/Game/MainMenu/SplashLevel");

// ── BP 이벤트 (BP 자식이 구현) ────────────────────────────────────────────────
protected:
	/**
	 * 참가자 목록 갱신 시 항목 위젯 초기화용.
	 * BP 에서 EntryWidget 을 WBP_PlayerEntry(UTDPlayerEntryWidget) 로 캐스트 후
	 * SetPlayerName(PlayerState.GetPlayerName()) 호출.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "TD|WaitingRoom")
	void OnInitPlayerEntry(UUserWidget* EntryWidget, ATDLobbyPlayerState* PlayerState);

// ── 내부 구현 ─────────────────────────────────────────────────────────────────
private:
	UFUNCTION()
	void OnStartClicked();

	UFUNCTION()
	void OnLeaveClicked();

	/** ATDLobbyGameState::OnPlayerSlotsChanged 콜백 → RefreshPlayerList 호출. */
	UFUNCTION()
	void HandlePlayerSlotsChanged();

	/** ScrollBox_players 를 현재 PlayerSlots 기준으로 다시 채움. */
	void RefreshPlayerList();

	UTDLobbySessionSubsystem* GetLobbySession() const;
	ATDLobbyPlayerController* GetLobbyPC() const;
};
