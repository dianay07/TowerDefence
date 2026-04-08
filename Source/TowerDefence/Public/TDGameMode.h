#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TDPoolActor.h"
#include "TDEventManagerComponent.h"
#include "TDWaveManagerComponent.h"
#include "TDGameMode.generated.h"

// 코인 관련 함수(HasCoins, SpendCoins, RefundCoins)는 ATDGameState에 있음
// 타워에서 직접 GameState->CoinChange() / GameState->HasCoins() 호출할 것

UCLASS()
class TOWERDEFENCE_API ATDGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Managers")
	UTDEventManagerComponent* EventManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Managers")
	UTDWaveManagerComponent* WaveManager;

public:
	ATDGameMode();

	void BeginPlay() override;

public:
	// ── Game State ────────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "Game")
	void GameEnded(bool bWin);

	UFUNCTION(BlueprintCallable, Category = "Game")
	void CheckIfWin();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void CheckIfLoss();

	// ── Pool ──────────────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable)
	AActor* GetPoolActorFromClass(TSubclassOf<AActor> ActorClass, FTransform Transform, AActor* NewOwner);

	UFUNCTION(BlueprintCallable)
	void PoolActor(AActor* PoolActor);

private:
	TMap<TSubclassOf<AActor>, TArray<AActor*>> ActorPool;
};
