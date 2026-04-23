#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TDTowerSpawnerComponent.generated.h"

class ATDTowerBase;
struct FStageRow;

/**
 * Tower 스폰 전담 서버 컴포넌트.
 * - 소유자: ATDGameMode (서버 전용)
 * - 책임:
 *     1) 스테이지 시작 시 TowerBase 일괄 스폰 (tile 스캔 → FStageRow 설정대로 스폰)
 *     2) 플레이어가 UI 에서 실제 Tower 선택 시 Base 를 교체하여 Tower 스폰
 */
UCLASS(ClassGroup=(TD), meta=(BlueprintSpawnableComponent))
class TOWERDEFENCE_API UTDTowerSpawnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDTowerSpawnerComponent();

	/**
	 * 서버 전용: FStageRow 설정을 사용해 TowerBase 를 일괄 스폰.
	 * - Row.TileMeshName 과 일치하는 메시를 가진 StaticMeshActor 를 월드에서 스캔
	 * - 각 타일 위치에 Row.TileSpawnZOffset 만큼 띄워서 Row.BaseTowerClass 스폰
	 * - 스테이지 초기화 시 1회만 호출하는 것이 전제.
	 * @param Row  스테이지 설정 (BaseTowerClass / TileMeshName / TileSpawnZOffset 사용)
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerSpawner")
	void SpawnInitialTowerBases(const FStageRow& Row);

	/**
	 * 서버 전용: Base 를 Destroy 하고 같은 Transform 에 NewClass 스폰.
	 * - 플레이어가 UI 에서 타워 클래스 선택 후 PlayerController Server RPC 경로로 호출 예정.
	 * - 현재는 UI 연결 전 스텁 (빈 구현). UI/RPC 플로우 확정 시 구현.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerSpawner")
	ATDTowerBase* SpawnTower(ATDTowerBase* BaseTower, TSubclassOf<ATDTowerBase> NewClass);
};
