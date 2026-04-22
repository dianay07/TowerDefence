#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TDPoolActor.h"
#include "TDEventManagerComponent.h"
#include "TDWaveManagerComponent.h"
#include "Server/TDEnemySpawnerComponent.h"
#include "Server/TDPoolComponent.h"
#include "TowerDefence/TD.h"
#include "TDGameMode.generated.h"

// 코인 관련 함수(HasCoins, SpendCoins, RefundCoins)는 ATDGameState에 있음
// 타워에서 직접 GameState->CoinChange() / GameState->HasCoins() 호출할 것

class ATowerManager;
class ATDEnemyActor;

UCLASS()
class TOWERDEFENCE_API ATDGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Managers")
	UTDEventManagerComponent* EventManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Managers")
	UTDWaveManagerComponent* WaveManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Managers")
	UTDEnemySpawnerComponent* EnemySpawner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Managers")
	UTDPoolComponent* Pool;

	// 레벨에 배치된 TowerManager — BeginPlay에서 자동 등록 (레벨 BP 수동 세팅 불필요)
	// 이름을 TowerManagerActor로 구분 — BP_GameMode의 기존 'TowerManager' 변수와 충돌 방지
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Managers")
	ATowerManager* TowerManagerActor;

	/** BP_GameMode 에서 EnemyType -> Class 매핑을 설정. BeginPlay에서 EnemyDataTableSubsystem에 주입. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	TMap<EEnemyType, TSubclassOf<ATDEnemyActor>> EnemyClasses;

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

};
