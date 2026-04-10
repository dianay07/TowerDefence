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

	// Phase 2에서 ReplicatedUsing=OnRep_* 으로 교체 예정
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	int32 SharedCoin = 2000;

	UPROPERTY(BlueprintReadOnly, Category = "Game")
	int32 BaseHealth = 20;

	UPROPERTY(BlueprintReadOnly, Category = "Game")
	int32 MaxBaseHealth = 20;

	UPROPERTY(BlueprintReadOnly, Category = "Game")
	int32 CurrentWave = 0;

public:
	// HearthHealthChaged 이벤트
	UPROPERTY(BlueprintAssignable, Category = "TD|Events")
	FOnBaseHealthChanged OnBaseHealthChanged;

	UFUNCTION(BlueprintCallable, Category = "TD|GameState")
	void DecreaseBaseHealth();

	UFUNCTION(BlueprintPure, Category = "TD|GameState")
	int32 GetCoins() const;

	UFUNCTION(BlueprintPure, Category = "TD|GameState")
	bool HasCoins(int32 Amount) const;

	// SharedCoinChanged 이벤트
	UPROPERTY(BlueprintAssignable, Category = "TD|Events")
	FOnCoinsChanged OnCoinsChanged;

	UFUNCTION(BlueprintCallable, Category = "TD|GameState")
	void CoinChange(int32 change);

	// GameEnded 이벤트
	UPROPERTY(BlueprintAssignable, Category = "TD|Events")
	FOnGameEnded OnGameEnded;

	void BroadcastGameEnded(bool bWin);
};
