#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TDPoolActor.h"
#include "TDWaveManagerComponent.h"
#include "Server/TDEnemySpawnerComponent.h"
#include "Server/TDTowerSpawnerComponent.h"
#include "Server/TDPoolComponent.h"
#include "Player/TDPlayerController.h"
#include "TowerDefence/TD.h"
#include "TDGameMode.generated.h"

// 코인 관련 함수(HasCoins, CoinChange)는 ATDGameState에 있음.
// 타워/컴포넌트는 GameState->CoinChange() / GameState->HasCoins() 를 직접 호출할 것.

class ATDEnemyActor;
struct FStageRow;

/**
 * TD 전용 GameMode. 서버 전용 게임 규칙 컴포넌트를 조립하고 생명주기를 관리한다.
 * - WaveManager / EnemySpawner / TowerSpawner / Pool 컴포넌트를 소유.
 * - PlayerController: ATDPlayerController (Client → Server RPC 진입점).
 * - 스테이지 초기화: LevelSessionSubsystem → InitializeStage(Row) → 각 컴포넌트 위임.
 */
UCLASS()
class TOWERDEFENCE_API ATDGameMode : public AGameModeBase
{
	GENERATED_BODY()

// ── 컴포넌트 ──────────────────────────────────────────────────────────────────
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Managers")
	UTDWaveManagerComponent* WaveManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Managers")
	UTDEnemySpawnerComponent* EnemySpawner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Managers")
	UTDTowerSpawnerComponent* TowerSpawner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Managers")
	UTDPoolComponent* Pool;

// ── 설정 ──────────────────────────────────────────────────────────────────────
public:
	/** BP_GameMode 에서 EnemyType → Class 매핑 설정. BeginPlay 에서 EnemyDataTableSubsystem 에 주입. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy")
	TMap<EEnemyType, TSubclassOf<ATDEnemyActor>> EnemyClasses;

// ── 생명주기 ──────────────────────────────────────────────────────────────────
public:
	ATDGameMode();
	virtual void BeginPlay() override;

// ── 게임 판정 ─────────────────────────────────────────────────────────────────
public:
	UFUNCTION(BlueprintCallable, Category = "Game")
	void GameEnded(bool bWin);

	UFUNCTION(BlueprintCallable, Category = "Game")
	void CheckIfWin();

	UFUNCTION(BlueprintCallable, Category = "Game")
	void CheckIfLoss();

// ── 액터 풀 (UTDPoolComponent 위임 래퍼 — BP 호환용) ─────────────────────────
public:
	UFUNCTION(BlueprintCallable)
	AActor* GetPoolActorFromClass(TSubclassOf<AActor> ActorClass, FTransform Transform, AActor* NewOwner);

	UFUNCTION(BlueprintCallable)
	void PoolActor(AActor* PoolActor);

// ── 스테이지 초기화 ───────────────────────────────────────────────────────────
public:
	/**
	 * 레벨 로드 완료 후 UTDLevelSessionSubsystem 이 호출.
	 * TowerSpawner::SpawnInitialTowerBases 등 각 컴포넌트에 초기화 위임.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|Stage")
	void InitializeStage(const FStageRow& Row);

};
