#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TowerDefence/TD.h"
#include "TDEnemyDataTableSubsystem.generated.h"

class ATDEnemyActor;

/**
 * Enemy 클래스 매핑(EEnemyType -> Subclass) 캐시 및 조회를 담당.
 * - 수명: GameInstance (앱 실행 중 유지)
 * - 권한: 로컬 readonly. 서버/클라 모두 동일하게 접근 가능.
 * - 데이터 주입: ATDGameMode::BeginPlay 에서 RegisterEnemyClasses() 로 공급.
 */
UCLASS()
class TOWERDEFENCE_API UTDEnemyDataTableSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** enum → Class 매핑 주입 (GameMode BeginPlay에서 호출). */
	UFUNCTION(BlueprintCallable, Category = "TD|EnemyDataTable")
	void RegisterEnemyClasses(const TMap<EEnemyType, TSubclassOf<ATDEnemyActor>>& InMap);

	/** 타입별 Enemy 클래스 조회. 없으면 nullptr. */
	UFUNCTION(BlueprintPure, Category = "TD|EnemyDataTable")
	TSubclassOf<ATDEnemyActor> GetEnemyClass(EEnemyType Type) const;

	UFUNCTION(BlueprintPure, Category = "TD|EnemyDataTable")
	bool IsLoaded() const { return EnemyClassMap.Num() > 0; }

	/** 현재 캐시된 엔트리 수 (디버그/검증용). */
	UFUNCTION(BlueprintPure, Category = "TD|EnemyDataTable")
	int32 GetEntryCount() const { return EnemyClassMap.Num(); }

private:
	UPROPERTY(Transient)
	TMap<EEnemyType, TSubclassOf<ATDEnemyActor>> EnemyClassMap;
};
