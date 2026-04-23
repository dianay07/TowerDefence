#include "Server/TDTowerSpawnerComponent.h"
#include "TDTowerBase.h"
#include "Session/TDLevelSessionSubsystem.h"  // FStageRow
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"

UTDTowerSpawnerComponent::UTDTowerSpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTDTowerSpawnerComponent::SpawnInitialTowerBases(const FStageRow& Row)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	TSubclassOf<ATDTowerBase> BaseClass = Row.BaseTowerClass.LoadSynchronous();
	if (!BaseClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TowerSpawner] BaseTowerClass not set for stage '%s'."),
			*Row.StageId.ToString());
		return;
	}

	// 월드에서 tile 메시를 가진 StaticMeshActor 수집
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), Found);

	const FString Target = Row.TileMeshName.ToString();
	const float ZOffset = Row.TileSpawnZOffset;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	int32 SpawnCount = 0;
	for (AActor* A : Found)
	{
		AStaticMeshActor* SMA = Cast<AStaticMeshActor>(A);
		if (!SMA) continue;

		UStaticMeshComponent* Comp = SMA->GetStaticMeshComponent();
		if (!Comp) continue;

		UStaticMesh* Mesh = Comp->GetStaticMesh();
		if (!Mesh) continue;

		if (!Mesh->GetName().Equals(Target, ESearchCase::IgnoreCase)) continue;

		FTransform T = SMA->GetActorTransform();
		FVector Loc = T.GetLocation();
		Loc.Z += ZOffset;
		T.SetLocation(Loc);

		if (World->SpawnActor<ATDTowerBase>(BaseClass, T, Params))
		{
			// ATDTowerBase::BeginPlay → GameState::RegisterTower 자동 호출
			++SpawnCount;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TowerSpawner] Stage '%s' → spawned %d TowerBase."),
		*Row.StageId.ToString(), SpawnCount);
}

ATDTowerBase* UTDTowerSpawnerComponent::SpawnTower(ATDTowerBase* BaseTower, TSubclassOf<ATDTowerBase> NewClass)
{
	// UI / PlayerController Server RPC 연결 전 스텁.
	// 예상 구현:
	//   1) HasAuthority / 유효성 체크
	//   2) const FTransform T = BaseTower->GetActorTransform();
	//   3) BaseTower->Destroy();  // EndPlay → GameState::UnregisterTower 자동
	//   4) return GetWorld()->SpawnActor<ATDTowerBase>(NewClass, T, Params);
	return nullptr;
}
