#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TDGameState.generated.h"

class ATDTowerBase;
class ATDEnemyActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBaseHealthChanged, int32, CurrentHealth, int32, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCoinsChanged, int32, Change, int32, Coin);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameEnded, bool, bWin);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDiedEvent, ATDEnemyActor*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyAttackedEvent, ATDEnemyActor*, Enemy, float, Damage);

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

// ── 배치된 타워 ───────────────────────────────────────────────────────────────
public:
	UPROPERTY(ReplicatedUsing=OnRep_PlacedTowers, BlueprintReadOnly, Category = "Game")
	TArray<ATDTowerBase*> PlacedTowers;

	UFUNCTION(BlueprintCallable, Category = "TD|GameState")
	void RegisterTower(ATDTowerBase* Tower);

	UFUNCTION(BlueprintCallable, Category = "TD|GameState")
	void UnregisterTower(ATDTowerBase* Tower);

private:
	UFUNCTION() void OnRep_PlacedTowers();

// ── 활성 적 목록 ──────────────────────────────────────────────────────────────
public:
	/** 현재 살아있는 Enemy 목록. 서버에서 관리, 클라이언트에 복제됨. */
	UPROPERTY(ReplicatedUsing=OnRep_ActiveEnemies, BlueprintReadOnly, Category = "Game")
	TArray<ATDEnemyActor*> ActiveEnemies;

	UFUNCTION(BlueprintCallable, Category = "TD|GameState")
	void RegisterEnemy(ATDEnemyActor* Enemy);

	UFUNCTION(BlueprintCallable, Category = "TD|GameState")
	void UnregisterEnemy(ATDEnemyActor* Enemy);

	/** 범위 내에서 경로를 가장 멀리 진행한 적 반환. 서버/클라 모두 호출 가능. */
	UFUNCTION(BlueprintCallable, Category = "TD|GameState")
	ATDEnemyActor* GetFurthestEnemy(FVector Location, float Radius) const;

	/** 범위 내 모든 적 반환. 서버/클라 모두 호출 가능. */
	UFUNCTION(BlueprintCallable, Category = "TD|GameState")
	TArray<ATDEnemyActor*> GetEnemiesInRange(FVector Location, float Radius) const;

private:
	UFUNCTION() void OnRep_ActiveEnemies();

// ── 적 이벤트 Multicast RPC ───────────────────────────────────────────────────
public:
	// BP/C++ 에서 구독 가능한 델리게이트 (클라이언트 측 UI/이펙트 연결용)
	UPROPERTY(BlueprintAssignable, Category = "TD|Events")
	FOnEnemyDiedEvent OnEnemyDied;

	UPROPERTY(BlueprintAssignable, Category = "TD|Events")
	FOnEnemyAttackedEvent OnEnemyAttacked;

	// 서버에서 호출 → 모든 클라이언트에 브로드캐스트
	UFUNCTION(BlueprintCallable, Category = "TD|GameState")
	void NotifyEnemyDied(ATDEnemyActor* Enemy);

	UFUNCTION(BlueprintCallable, Category = "TD|GameState")
	void NotifyEnemyAttacked(ATDEnemyActor* Enemy, float Damage);

private:
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnEnemyDied(ATDEnemyActor* Enemy);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnEnemyAttacked(ATDEnemyActor* Enemy, float Damage);

// ── 게임 종료 ─────────────────────────────────────────────────────────────────
public:
	UPROPERTY(BlueprintAssignable, Category = "TD|Events")
	FOnGameEnded OnGameEnded;

	void BroadcastGameEnded(bool bWin);
};
