#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TowerDefence/TD.h"
#include "TDTowerDataTableSubsystem.generated.h"

class UDataTable;

/**
 * 타워 정적 데이터(DataTable) 캐시 및 조회를 담당.
 * - 수명: GameInstance (앱 실행 중 유지)
 * - 권한: 로컬 readonly. 서버/클라 모두 동일하게 접근 가능.
 * - 데이터 주입: GameInstance 또는 GameMode 등 호출자가 LoadFromDataTable() 로 공급.
 */
UCLASS()
class TOWERDEFENCE_API UTDTowerDataTableSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 외부(GameInstance/GameMode)에서 DT 를 주입. 성공 시 true. */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerDataTable")
	bool LoadFromDataTable(UDataTable* DataTable);

	/** 기본 타워 클래스(BP_BaseTower) 주입. 배치 요청 시 어디서 스폰할지 참조용. */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerDataTable")
	void SetBaseTowerClass(TSubclassOf<AActor> InClass) { BaseTowerClass = InClass; }

	/** 타입별 타워 데이터 조회. 없으면 false. */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerDataTable")
	bool GetTowerData(ETowerType TowerType, FTowerData& OutTowerData) const;

	UFUNCTION(BlueprintPure, Category = "TD|TowerDataTable")
	TSubclassOf<AActor> GetBaseTowerClass() const { return BaseTowerClass; }

	UFUNCTION(BlueprintPure, Category = "TD|TowerDataTable")
	bool IsLoaded() const { return bLoaded; }

	/** 현재 캐시된 엔트리 수 (디버그/검증용). */
	UFUNCTION(BlueprintPure, Category = "TD|TowerDataTable")
	int32 GetEntryCount() const { return TowerDataMap.Num(); }

private:
	UPROPERTY(Transient)
	TMap<ETowerType, FTowerData> TowerDataMap;

	UPROPERTY(Transient)
	TSubclassOf<AActor> BaseTowerClass;

	bool bLoaded = false;
};
