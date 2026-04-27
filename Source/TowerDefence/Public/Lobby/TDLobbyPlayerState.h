#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TDLobbyPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTDOnReadyChanged);

/**
 * 로비 전용 PlayerState.
 * - DisplayName: 부모 APlayerState::GetPlayerName() 재사용.
 * - bIsReady: 준비 완료 플래그, 복제.
 */
UCLASS()
class TOWERDEFENCE_API ATDLobbyPlayerState : public APlayerState
{
	GENERATED_BODY()

// ── 생명주기 ──────────────────────────────────────────────────────────────────
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

// ── 준비 상태 ─────────────────────────────────────────────────────────────────
public:
	/** 서버에서 호출. bIsReady 를 설정하고 복제. */
	void SetReady(bool bReady);

	UPROPERTY(ReplicatedUsing = OnRep_bIsReady, BlueprintReadOnly, Category = "TD|Lobby")
	bool bIsReady = false;

	/** 복제 변경 시 위젯에 알리는 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "TD|Lobby")
	FTDOnReadyChanged OnReadyChanged;

private:
	UFUNCTION()
	void OnRep_bIsReady();
};
