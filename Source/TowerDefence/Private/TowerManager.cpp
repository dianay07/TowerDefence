// Fill out your copyright notice in the Description page of Project Settings.


#include "TowerManager.h"

#include "Kismet/GameplayStatics.h" // For GetAllActorsOfClass
#include "Engine/StaticMeshActor.h" // For AStaticMeshActor Class Reference
#include "TDTowerPawn.h" // For AStaticMeshActor Class Reference
#include "TDFL_Utility.h" // For BP Util
#include "Kismet/DataTableFunctionLibrary.h" // Get DataTable
#include "TDWaveManagerComponent.h"

// Sets default values
ATowerManager::ATowerManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void ATowerManager::SpawnTowers()
{
	if (TowerLocations.IsEmpty())
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(
			GetWorld(),
			AStaticMeshActor::StaticClass(),   // 찾을 클래스
			FoundActors              // 결과 담을 배열
		);

		for (AActor* Actor : FoundActors)
		{
			AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(Actor);
			if (!IsValid(StaticMeshActor)) continue;
			
			UStaticMeshComponent* StaticMeshComp = StaticMeshActor->GetStaticMeshComponent();
			if (!IsValid(StaticMeshComp)) continue;

			UStaticMesh* StaticMesh = StaticMeshComp->GetStaticMesh();
			if (!IsValid(StaticMesh)) continue;

			// 해당 타일에 타워를 지을 수 있는데 다른 방식으로 할 수 있게 변경 하는게 좋을 것 같음
			// 왜? - 다른 타일을 쓸 수도 있으닌깐
			if(StaticMesh->GetName().Equals(TEXT("tile-dirt"), ESearchCase::IgnoreCase))
			{
				TowerLocations.Add(Actor);
			}
			
		}
	}

	for (AActor* Actor : TowerLocations)
	{
		FVector ActorLocation = Actor->GetTargetLocation();
		ActorLocation.Z += 10;
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		GetWorld()->SpawnActor<AActor>(BP_BaseTowerClass, ActorLocation, Actor->GetActorRotation(), SpawnParams);
		// BP_BaseTowerClass 용 클래스라서 BP_BaseTowerClass에 접근해서 visible 관련 로직은 BP_BaseTowerClass를 C++ 변경후 처리
		// 혹은 BP 에서 처리 한다.
	}

}

void ATowerManager::UpdateHightlight()
{
	// Sequence Then 0: Get Tower Under Mouse
	ATDTowerPawn* TowerResult = UTDFL_Utility::GetTowerUnderMouse(GetWorld());
	ATDTowerPawn* NewTower = nullptr;
	if (TowerResult)
	{
		NewTower = TowerResult;
	}

	// Sequence Then 1: NewTower != HighlightedTower 체크
	if (NewTower == HighlightedTower) return;

	// 이전에 선택된 타워 하이라이트 끄기
	if (IsValid(HighlightedTower))
	{
		//HighlightedTower->SetHighlight(false);
		HighlightedTower->SetHidden(false);
	}

	// 새로 선택된 타워 하이라이트 켜기
	HighlightedTower = NewTower;
	if (IsValid(HighlightedTower))
	{
		//HighlightedTower->SetHighlight(true);
		HighlightedTower->SetHidden(true);
	}
}

void ATowerManager::ImportData()
{
	TArray<FName> OutRowNames;
	if (!IsValid(BP_DT_TowerData.Get())) return;

	UDataTableFunctionLibrary::GetDataTableRowNames(BP_DT_TowerData.Get(), OutRowNames);
	// Get Data Table Row Names
	TArray<FName> RowNames = BP_DT_TowerData.Get()->GetRowNames();

	// For Each Loop
	for (const FName& RowName : RowNames)
	{
		// Get Data Table Row BP_DT_TowerData
		FTowerData* Row = BP_DT_TowerData.Get()->FindRow<FTowerData>(
			RowName,
			TEXT("ImportData")
		);

		// Row Not Found
		if (!Row) continue;

		// Row Found → Add
		TowerData.Add(Row->EnemyType, *Row);
	}
}

bool ATowerManager::GetTowerData(ETowerType TowerType, FTowerData& OutTowerData)
{
	FTowerData* Found = TowerData.Find(TowerType);
	if (!Found)
	{
		return false;
	}

	OutTowerData = *Found;
	return true;
}


// Called when the game starts or when spawned
void ATowerManager::BeginPlay()
{
	Super::BeginPlay();
	SpawnTowers();
	ImportData();
}

// Called every frame
void ATowerManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateHightlight();
}



