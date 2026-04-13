#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../TD.h"
#include "TDWaveManagerComponent.generated.h"

class ATDPath;
class ATDEnemyActor;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOWERDEFENCE_API UTDWaveManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDWaveManagerComponent();

	void BeginPlay() override;
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

// ── 경로 ──────────────────────────────────────────────────────────────────────
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wave")
	TArray<ATDPath*> Paths;

	UPROPERTY(BlueprintReadWrite, Category = "Wave")
	TArray<float> PathLengths;

// ── 적 목록 ───────────────────────────────────────────────────────────────────
public:
	UPROPERTY(BlueprintReadWrite, Category = "Wave")
	TArray<ATDEnemyActor*> Enemies;

	UPROPERTY(BlueprintReadOnly, Category = "Wave")
	TArray<ATDEnemyActor*> ExpiredEnemies;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TMap<EEnemyType, TSubclassOf<ATDEnemyActor>> EnemyTypeClass;

// ── 웨이브 데이터 ─────────────────────────────────────────────────────────────
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	UDataTable* DataTable;

	UPROPERTY(BlueprintReadWrite, Category = "Wave")
	TArray<FWaveData> WaveData;

// ── 카운트 ────────────────────────────────────────────────────────────────────
public:
	UPROPERTY(BlueprintReadWrite, Category = "Wave")
	float TotalEnemyCount = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "Wave")
	float KillCount = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "Wave")
	float TimeUntilEnemy = 0.f;

// ── 웨이브 진행 함수 ──────────────────────────────────────────────────────────
public:
	UFUNCTION(BlueprintCallable, Category = "Wave")
	void UpdateWave(float Delta);

	UFUNCTION(BlueprintCallable, Category = "Wave")
	void AdvanceEnemies(float Delta);

	UFUNCTION(BlueprintCallable, Category = "Wave")
	void ImportData();

	UFUNCTION(BlueprintCallable, Category = "Wave")
	void SpawnEnemy(EEnemyType EnemyType);

	UFUNCTION(BlueprintCallable, Category = "Wave")
	void OnEnemyDied(ATDEnemyActor* Enemy);

	UFUNCTION(BlueprintPure, Category = "Wave")
	bool DoEnemiesRemain() const;

// ── 적 조회 함수 ──────────────────────────────────────────────────────────────
public:
	UFUNCTION(BlueprintCallable, Category = "Wave")
	ATDEnemyActor* GetFurthestEnemy(FVector Location, float Radius);

	UFUNCTION(BlueprintCallable, Category = "Wave")
	TArray<ATDEnemyActor*> GetEnemiesInRange(FVector Location, float Radius);
};
