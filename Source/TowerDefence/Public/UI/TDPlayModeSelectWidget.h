#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TDPlayModeSelectWidget.generated.h"

class UButton;

/**
 * 솔로/멀티 모드 선택 위젯의 C++ 베이스 (WBP_PlayModeSelect 의 부모).
 *
 * 책임 분리:
 *   - C++ 베이스 : 버튼 바인딩, 스테이지 ID 보관, 솔로 진입(LevelSessionSubsystem),
 *                  멀티 진입(LobbySessionSubsystem), Back 요청
 *   - BP 자식    : UMG 레이아웃, 스테이지 이름 표시, 버튼 스타일,
 *                  OnMultiModeRequested → WBP_MultiLobby 표시
 *                  OnBackRequested     → 이전 UI 복구
 *
 * 사용 흐름:
 *   1) 부모 위젯이 InitWithStage(StageId) 호출 + 이 위젯 표시
 *   2) Solo 클릭 → RequestSoloMode() → LevelSessionSubsystem::RequestLoadStage
 *   3) Multi 클릭 → RequestMultiMode() → LobbySubsystem::SetMultiStageId → OnMultiModeRequested 발화
 *   4) Back 클릭 → OnBackRequested 발화
 */
UCLASS(Abstract)
class TOWERDEFENCE_API UTDPlayModeSelectWidget : public UUserWidget
{
	GENERATED_BODY()

// ── 생명주기 ──────────────────────────────────────────────────────────────────
protected:
	virtual void NativeConstruct() override;

// ── UI 바인딩 (BP 위젯 변수명과 정확히 일치) ──────────────────────────────────
protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> SoloModeButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> MultiModeButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> BackButton;

// ── 공개 API ──────────────────────────────────────────────────────────────────
public:
	/**
	 * 레벨 버튼 클릭 시 부모 위젯에서 호출.
	 * 선택한 스테이지 ID를 보관하고 OnStageSet 이벤트를 발화한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|PlayMode")
	void InitWithStage(FName InStageId);

	/** Solo 버튼 → LevelSessionSubsystem::RequestLoadStage(SelectedStageId) */
	UFUNCTION(BlueprintCallable, Category = "TD|PlayMode")
	void RequestSoloMode();

	/**
	 * Multi 버튼 → LobbySessionSubsystem 에 StageId 저장 후 OnMultiModeRequested 발화.
	 * BP 자식에서 OnMultiModeRequested 를 구현해 WBP_MultiLobby 를 표시한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|PlayMode")
	void RequestMultiMode();

	/** 현재 선택된 스테이지 ID. BP 에서 이름 표시 등에 활용. */
	UFUNCTION(BlueprintPure, Category = "TD|PlayMode")
	FName GetSelectedStageId() const { return SelectedStageId; }

// ── BP 이벤트 (BP 자식이 구현) ────────────────────────────────────────────────
protected:
	/**
	 * InitWithStage() 완료 시 발화.
	 * BP 에서 스테이지 이름/이미지 등 UI 갱신.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "TD|PlayMode")
	void OnStageSet(FName StageId);

	/**
	 * RequestMultiMode() 완료 시 발화.
	 * BP 에서 WBP_MultiLobby 를 CreateWidget + AddToViewport.
	 * (자신을 SetVisibility Hidden 하거나 RemoveFromParent 해도 됨)
	 * @param StageId  LobbySessionSubsystem 에 이미 저장된 스테이지 ID
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "TD|PlayMode")
	void OnMultiModeRequested(FName StageId);

	/**
	 * Back 버튼 클릭 시 발화.
	 * BP 에서 이 위젯을 숨기고 이전 UI(스테이지 선택 등) 복구.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "TD|PlayMode")
	void OnBackRequested();

// ── 내부 상태 / 구현 ──────────────────────────────────────────────────────────
private:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "TD|PlayMode", meta = (AllowPrivateAccess = "true"))
	FName SelectedStageId;

	UFUNCTION() void OnSoloModeClicked();
	UFUNCTION() void OnMultiModeClicked();
	UFUNCTION() void OnBackClicked();
};
