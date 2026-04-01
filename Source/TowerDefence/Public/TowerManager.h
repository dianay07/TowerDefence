// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TowerDefence/TD.h"
#include "Engine/DataTable.h"

#include "TowerManager.generated.h"

UCLASS()
class TOWERDEFENCE_API ATowerManager : public AActor
{
	GENERATED_BODY()
	
private:
	TArray<TObjectPtr<AActor>> TowerLocations;
	TObjectPtr<AActor> HighlightedTower;
	TMap<ETowerType, FTowerData> TowerData;
	// DataTable

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSubclassOf<AActor> BP_BaseTowerClass;  // 에디터에서 BP 클래스 지정

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSoftObjectPtr<UDataTable> BP_DT_TowerData;  // 에디터에서 BP 클래스 지정

public:	
	// Sets default values for this actor's properties
	ATowerManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "TowerManager")
	void SpawnTowers();
	UFUNCTION(BlueprintCallable, Category = "TowerManager")
	void UpdateHightlight();
	UFUNCTION(BlueprintCallable, Category = "TowerManager")
	void ImportData();
	UFUNCTION(BlueprintCallable, Category = "TowerManager")
	bool GetTowerData(ETowerType TowerType, FTowerData& OutTowerData);
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	
};
