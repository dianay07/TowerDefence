#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Session/TDLobbySessionSubsystem.h"  // FTDSessionInfo
#include "TDSessionEntryWidget.generated.h"

class UButton;
class UTextBlock;
class UTDLobbySessionSubsystem;

/**
 * 세션 검색 결과 한 줄 위젯 C++ 베이스 (WBP_SessionEntry 의 부모).
 *
 * 책임:
 *   - InitWithSessionInfo : FTDSessionInfo 전체를 한번에 설정 (권장)
 *   - SetHostName         : 호스트 이름만 단독 설정 (BP 호환용)
 *   - SetSessionIndex     : 검색 결과 인덱스만 단독 설정 (BP 호환용)
 *   - Button_Join 클릭    : GetLobbySession → JoinSession(CachedSessionIndex)
 *
 * UTDMultiLobbyWidget::OnInitSessionEntry 에서 이 위젯에 InitWithSessionInfo 호출.
 */
UCLASS(Abstract)
class TOWERDEFENCE_API UTDSessionEntryWidget : public UUserWidget
{
	GENERATED_BODY()

// ── 생명주기 ──────────────────────────────────────────────────────────────────
protected:
	virtual void NativeConstruct() override;

// ── UI 바인딩 (BP 위젯 변수명과 정확히 일치) ──────────────────────────────────
protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_HostName;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Text_Players;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Button_Join;

// ── 공개 API ──────────────────────────────────────────────────────────────────
public:
	/**
	 * FTDSessionInfo 를 받아 모든 표시 정보를 한번에 설정 (권장).
	 * UTDMultiLobbyWidget::OnInitSessionEntry BP 구현에서 호출.
	 *   · Text_HostName  ← Info.OwnerName
	 *   · Text_Players   ← "(현재 인원) / (최대 인원)" 포맷
	 *   · CachedIndex    ← Info.SearchResultIndex (Join 시 사용)
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|SessionEntry")
	void InitWithSessionInfo(const FTDSessionInfo& Info);

	/**
	 * 호스트 이름만 단독 설정 → Text_HostName 갱신.
	 * (BP 에서 SetHostName / SetSessionIndex 를 각각 호출하는 경우 호환용)
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|SessionEntry")
	void SetHostName(const FString& Name);

	/**
	 * 검색 결과 인덱스만 단독 설정.
	 * Join 버튼 클릭 시 이 값을 JoinSession(Index) 에 전달한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|SessionEntry")
	void SetSessionIndex(int32 Index);

// ── 내부 상태 / 구현 ──────────────────────────────────────────────────────────
private:
	/** JoinSession 호출 시 사용할 검색 결과 인덱스. */
	UPROPERTY(Transient)
	int32 CachedSessionIndex = -1;

	UFUNCTION()
	void OnJoinClicked();

	UTDLobbySessionSubsystem* GetLobbySession() const;
};
