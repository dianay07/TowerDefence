#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TDPlayerEntryWidget.generated.h"

class UTextBlock;

/**
 * 대기실 참가자 한 줄 위젯 C++ 베이스 (WBP_PlayerEntry 의 부모).
 *
 * 책임:
 *   - SetPlayerName : Text_PlayerName 텍스트 설정
 *
 * UTDWaitingRoomWidget::OnInitPlayerEntry 에서
 * EntryWidget 을 이 클래스로 캐스트 후 SetPlayerName 호출.
 */
UCLASS(Abstract)
class TOWERDEFENCE_API UTDPlayerEntryWidget : public UUserWidget
{
	GENERATED_BODY()

// ── UI 바인딩 (BP 위젯 변수명과 정확히 일치) ──────────────────────────────────
protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_PlayerName;

// ── 공개 API ──────────────────────────────────────────────────────────────────
public:
	/** 참가자 이름 설정 → Text_PlayerName 갱신. */
	UFUNCTION(BlueprintCallable, Category = "TD|PlayerEntry")
	void SetPlayerName(const FString& Name);
};
