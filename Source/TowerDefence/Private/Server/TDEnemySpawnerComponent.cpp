#include "Server/TDEnemySpawnerComponent.h"
#include "Enemy/TDEnemyActor.h"
#include "Wave/TDPathActor.h"
#include "TDFL_Utility.h"
#include "GameData/TDEnemyDataTableSubsystem.h"

UTDEnemySpawnerComponent::UTDEnemySpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

ATDEnemyActor* UTDEnemySpawnerComponent::SpawnEnemy(EEnemyType Type, ATDPath* Path)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return nullptr;
	if (!IsValid(Path)) return nullptr;

	UTDEnemyDataTableSubsystem* DT = UTDFL_Utility::GetEnemyDataTable(this);
	if (!DT) return nullptr;

	TSubclassOf<ATDEnemyActor> EnemyClass = DT->GetEnemyClass(Type);
	if (!EnemyClass) return nullptr;

	FTransform SpawnTransform(Path->GetLocation(0.f));
	ATDEnemyActor* Enemy = GetWorld()->SpawnActor<ATDEnemyActor>(EnemyClass, SpawnTransform);
	if (!IsValid(Enemy)) return nullptr;

	Enemy->InitializePath(Path);
	return Enemy;
}
