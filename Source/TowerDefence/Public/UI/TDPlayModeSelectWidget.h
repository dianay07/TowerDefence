#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TDPlayModeSelectWidget.generated.h"

/**
 * 솔로/멀티 모드 선택 위젯의 C++ 베이스.
 *
 * 책임 분리:
 *   - C++ 베이스: 스테이지 ID 보관, 솔로 진입(LevelSessionSubsystem 위임), 멀티 진입(LobbySession 위임)
 *   - BP 자식 (WBP_PlayModeSelect): UMG 레이아웃, 스테이지 이름 표시, 버튼 스타일
 *
 * 사용 흐름:
 *   1) WBP_LevelSelectButton 클릭 → 부모 위젯이 InitWithStage(StageId) 호출 + 이 위젯 표시
 *   2) Solo 버튼 OnClicked → RequestSoloMode()  → LevelSessionSubsystem::RequestLoadStage
 *   3) Multi 버튼 OnClicked → RequestMultiMode() → LobbySessionSubsystem::CreateSession (TODO)
 */
UCLASS(Abstract)
class TOWERDEFENCE_API UTDPlayModeSelectWidget : public UUserWidget
{
	GENERATED_BODY()

// ── 공개 API ──────────────────────────────────────────────────────────────────
public:
	/**
	 * 레벨 버튼 클릭 시 부모 위젯에서 호출.
	 * 선택한 스테이지 ID를 보관하고 OnStageSet 이벤트를 발화한다.
	 * @param InStageId  DT_Stages 의 StageId 필드 값 (예: "Stage_01")
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|PlayMode")
	void InitWithStage(FName InStageId);

	/**
	 * Solo 버튼 OnClicked 에서 호출.
	 * LevelSessionSubsystem::RequestLoadStage(SelectedStageId) 로 게임 레벨 진입.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|PlayMode")
	void RequestSoloMode();

	/**
	 * Multi 버튼 OnClicked 에서 호출.
	 * LobbySessionSubsystem::CreateSession 으로 로비 세션 생성 후 로비 레벨 진입.
	 * TODO: LobbySessionSubsystem 연동 (CLAUDE.md §11 완료 후)
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|PlayMode")
	void RequestMultiMode();

	/** 현재 선택된 스테이지 ID. BP 에서 이름 표시 등에 활용. */
	UFUNCTION(BlueprintPure, Category = "TD|PlayMode")
	FName GetSelectedStageId() const { return SelectedStageId; }

// ── BP 인터페이스 (BP 자식이 구현) ────────────────────────────────────────────
protected:
	/**
	 * InitWithStage() 완료 시 발화.
	 * BP 자식에서 스테이지 이름/이미지 등 UI 갱신에 사용.
	 * @param StageId  설정된 스테이지 ID
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "TD|PlayMode")
	void OnStageSet(FName StageId);

// ── 내부 상태 ─────────────────────────────────────────────────────────────────
private:
	/** InitWithStage 에서 저장한 선택 스테이지 ID. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "TD|PlayMode", meta = (AllowPrivateAccess = "true"))
	FName SelectedStageId;
};
