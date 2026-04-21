#include "GameData/TDTowerDataTableSubsystem.h"
#include "Engine/DataTable.h"

void UTDTowerDataTableSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UTDTowerDataTableSubsystem::Deinitialize()
{
	TowerDataMap.Empty();
	BaseTowerClass = nullptr;
	bLoaded = false;
	Super::Deinitialize();
}

bool UTDTowerDataTableSubsystem::LoadFromDataTable(UDataTable* DataTable)
{
	if (!IsValid(DataTable))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TowerDataTable] LoadFromDataTable: invalid DataTable."));
		return false;
	}

	TowerDataMap.Reset();

	for (const FName& RowName : DataTable->GetRowNames())
	{
		FTowerData* Row = DataTable->FindRow<FTowerData>(RowName, TEXT("TDTowerDataTableSubsystem"));
		if (!Row) continue;

		if (Row->EnemyType == ETowerType::None)
		{
			UE_LOG(LogTemp, Warning, TEXT("[TowerDataTable] Row %s has EnemyType=None, skipped."),
				*RowName.ToString());
			continue;
		}

		TowerDataMap.Add(Row->EnemyType, *Row);
	}

	bLoaded = TowerDataMap.Num() > 0;

	UE_LOG(LogTemp, Log, TEXT("[TowerDataTable] Loaded %d entries from %s."),
		TowerDataMap.Num(), *DataTable->GetName());

	return bLoaded;
}

bool UTDTowerDataTableSubsystem::GetTowerData(ETowerType TowerType, FTowerData& OutTowerData) const
{
	if (const FTowerData* Found = TowerDataMap.Find(TowerType))
	{
		OutTowerData = *Found;
		return true;
	}
	return false;
}
