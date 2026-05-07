#include "Server/TDTowerSpawnerComponent.h"
#include "Tower/TDTowerBase.h"
#include "TDGameState.h"
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

void UTDTowerSpawnerComponent::DoTowerAction(ATDTowerBase* Tower, ETowerActions Action)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority()) return;
	if (!IsValid(Tower)) return;

	UWorld* World = GetWorld();
	if (!World) return;

	ATDGameState* GameState = Cast<ATDGameState>(UGameplayStatics::GetGameState(this));

	// 비용 / 환급 계산
	int32 CostOrRefund = 0;
	FString Description;
	Tower->GetTowerDetails(Action, CostOrRefund, Description);

	TSubclassOf<AActor> NewTowerClass = nullptr;

	if (Action == ETowerActions::BreakDown)
	{
		// 철거: 코인 환급 후 빈 타워로 교체
		if (IsValid(GameState))
		{
			GameState->CoinChange(CostOrRefund);
		}
		NewTowerClass = Tower->BaseTowerClass;
	}
	else
	{
		// 건설 / 업그레이드: 코인 잔액 검증 후 차감
		if (!IsValid(GameState) || !GameState->HasCoins(CostOrRefund)) return;
		GameState->CoinChange(-CostOrRefund);
	}

	switch (Action)
	{
	case ETowerActions::BuildTurret:    NewTowerClass = Tower->TurretClass;   break;
	case ETowerActions::BuildBallista:  NewTowerClass = Tower->BallistaClass; break;
	case ETowerActions::BuildCatapult:  NewTowerClass = Tower->CatapultClass; break;
	case ETowerActions::BuildCannon:    NewTowerClass = Tower->CannonClass;   break;
	case ETowerActions::BreakDown:      /* NewTowerClass = BaseTowerClass 위에서 설정됨 */ break;
	case ETowerActions::Upgrade:
		Tower->UpgradeTower();
		return;  // 업그레이드는 자기 자신을 교체하지 않음
	default:
		return;
	}

	if (!NewTowerClass) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* NewTower = World->SpawnActor<AActor>(NewTowerClass, Tower->GetActorTransform(), SpawnParams);
	if (IsValid(NewTower))
	{
		Tower->UnSelect();
		Tower->Destroy();
	}
}
