#include "GameData/TDEnemyDataTableSubsystem.h"

void UTDEnemyDataTableSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UTDEnemyDataTableSubsystem::Deinitialize()
{
	EnemyClassMap.Empty();
	Super::Deinitialize();
}

void UTDEnemyDataTableSubsystem::RegisterEnemyClasses(const TMap<EEnemyType, TSubclassOf<ATDEnemyActor>>& InMap)
{
	EnemyClassMap = InMap;

	UE_LOG(LogTemp, Log, TEXT("[UTDEnemyDataTableSubsystem] RegisterEnemyClasses: %d entries registered."), EnemyClassMap.Num());
}

TSubclassOf<ATDEnemyActor> UTDEnemyDataTableSubsystem::GetEnemyClass(EEnemyType Type) const
{
	const TSubclassOf<ATDEnemyActor>* Found = EnemyClassMap.Find(Type);
	return Found ? *Found : nullptr;
}
