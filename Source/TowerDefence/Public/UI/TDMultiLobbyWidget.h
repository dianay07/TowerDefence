#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Session/TDLobbySessionSubsystem.h"
#include "TDMultiLobbyWidget.generated.h"

class UButton;
class UTextBlock;
class UScrollBox;
class UTDLobbySessionSubsystem;

/**
 * 멀티 세션 브라우저 위젯 C++ 베이스 (WBP_MultiLobby 의 부모).
 *
 * 책임:
 *   - Create Room: CreateSession → OnCreateRoomSuccess (BP 에서 레벨 이동 처리)
 *   - Refresh    : FindSessions → 결과를 SessionsScrollBox 에 SessionEntryWidgetClass 로 채움
 *   - Back       : OnBackRequested (BP 에서 이전 UI 복구)
 *
 * BP 자식 (WBP_MultiLobby) 이 구현할 것:
 *   - OnInitSessionEntry : WBP_SessionEntry 의 SetSessionIndex / SetHostName 호출
 *   - OnCreateRoomSuccess: OpenLevel(MainMenuLevel?listen) 또는 WBP_WaitingRoom 표시
 *   - OnBackRequested    : 이전 위젯 복구 (SetVisibility / RemoveFromParent)
 */
UCLASS(Abstract)
class TOWERDEFENCE_API UTDMultiLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

// ── 생명주기 ──────────────────────────────────────────────────────────────────
protected:
	virtual void NativeConstruct() override;

// ── UI 바인딩 (이름이 BP 위젯 변수명과 정확히 일치해야 함) ──────────────────────
protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> CreateRoomButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> RefreshButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> BackButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> SessionsScrollBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> StatusText;

// ── 설정 ──────────────────────────────────────────────────────────────────────
public:
	/**
	 * 방 목록 한 줄 위젯 클래스. BP_MultiLobby Details 에서 WBP_SessionEntry 지정.
	 * Refresh 후 SessionsScrollBox 에 자동 생성됨.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "TD|MultiLobby")
	TSubclassOf<UUserWidget> SessionEntryWidgetClass;

// ── BP 이벤트 (BP 자식이 구현) ────────────────────────────────────────────────
protected:
	/**
	 * Refresh 완료 후 항목 위젯 초기화 시 발화.
	 * BP 에서 EntryWidget 을 WBP_SessionEntry(UTDSessionEntryWidget) 로 캐스트 후
	 * InitWithSessionInfo(Info) 를 호출하면 한번에 모든 정보가 설정됨.
	 * (또는 SetSessionIndex / SetHostName 개별 호출도 가능)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "TD|MultiLobby")
	void OnInitSessionEntry(UUserWidget* EntryWidget, const FTDSessionInfo& Info);

	/**
	 * CreateSession 성공 시 발화.
	 * BP 에서 OpenLevel("MainMenuLevel", Options="?listen") 호출.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "TD|MultiLobby")
	void OnCreateRoomSuccess();

	/**
	 * Back 버튼 클릭 시 발화.
	 * BP 에서 이 위젯을 숨기고 PlayModeSelect 위젯을 복구.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "TD|MultiLobby")
	void OnBackRequested();

// ── 내부 구현 ─────────────────────────────────────────────────────────────────
private:
	UFUNCTION()
	void OnCreateRoomClicked();

	UFUNCTION()
	void OnRefreshClicked();

	UFUNCTION()
	void OnBackClicked();

	UFUNCTION()
	void HandleSessionCreated(bool bWasSuccessful);

	UFUNCTION()
	void HandleSessionsFound(bool bWasSuccessful);

	UTDLobbySessionSubsystem* GetLobbySession() const;
};
