#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TDGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBaseHealthChanged, int32, CurrentHealth, int32, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCoinsChanged, int32, Change, int32, Coin);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameEnded, bool, bWin);

UCLASS()
class TOWERDEFENCE_API ATDGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ATDGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

// ── 게임 데이터 ───────────────────────────────────────────────────────────────
public:
	UPROPERTY(ReplicatedUsing=OnRep_SharedCoin, BlueprintReadOnly, Category = "Game")
	int32 SharedCoin = 2000;

	UPROPERTY(ReplicatedUsing=OnRep_BaseHealth, BlueprintReadOnly, Category = "Game")
	int32 BaseHealth = 20;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game")
	int32 MaxBaseHealth = 20;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentWave, BlueprintReadOnly, Category = "Game")
	int32 CurrentWave = 0;

private:
	UFUNCTION() void OnRep_SharedCoin();
	UFUNCTION() void OnRep_BaseHealth();
	UFUNCTION() void OnRep_CurrentWave();

// ── 기지 체력 ─────────────────────────────────────────────────────────────────
public:
	UPROPERTY(BlueprintAssignable, Category = "TD|Events")
	FOnBaseHealthChanged OnBaseHealthChanged;

	UFUNCTION(BlueprintCallable, Category = "TD|GameState")
	void DecreaseBaseHealth();

// ── 코인 ──────────────────────────────────────────────────────────────────────
public:
	UPROPERTY(BlueprintAssignable, Category = "TD|Events")
	FOnCoinsChanged OnCoinsChanged;

	UFUNCTION(BlueprintPure, Category = "TD|GameState")
	int32 GetCoins() const;

	UFUNCTION(BlueprintPure, Category = "TD|GameState")
	bool HasCoins(int32 Amount) const;

	UFUNCTION(BlueprintCallable, Category = "TD|GameState")
	void CoinChange(int32 change);

// ── 게임 종료 ─────────────────────────────────────────────────────────────────
public:
	UPROPERTY(BlueprintAssignable, Category = "TD|Events")
	FOnGameEnded OnGameEnded;

	void BroadcastGameEnded(bool bWin);
};
