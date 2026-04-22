#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TowerDefence/TD.h"
#include "TDEnemySpawnerComponent.generated.h"

class ATDEnemyActor;
class ATDPath;

/**
 * Enemy 스폰 전담 서버 컴포넌트.
 * - 소유자: ATDGameMode (서버 전용)
 * - 책임: EEnemyType + ATDPath를 받아 Enemy Actor를 SpawnActor 후 반환.
 * - OnDied 바인딩은 호출자(UTDWaveManagerComponent)가 반환값으로 연결.
 */
UCLASS(ClassGroup=(TD), meta=(BlueprintSpawnableComponent))
class TOWERDEFENCE_API UTDEnemySpawnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDEnemySpawnerComponent();

	/**
	 * 서버 전용: 주어진 경로 시작점에 EnemyType에 대응하는 Enemy를 스폰.
	 * @param Type     스폰할 Enemy 종류
	 * @param Path     경로 Actor — InitializePath 에 전달됨
	 * @return         스폰된 ATDEnemyActor*, 실패 시 nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|EnemySpawner")
	ATDEnemyActor* SpawnEnemy(EEnemyType Type, ATDPath* Path);
};
