#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TDPoolComponent.generated.h"

/**
 * Actor Pool 전담 서버 컴포넌트.
 * - 소유자: ATDGameMode (서버 전용)
 * - 책임: ITDPoolActorInterface 구현 Actor의 스폰/재사용/반납 관리.
 * - 기존 ATDGameMode의 GetPoolActorFromClass / PoolActor 로직을 이전.
 */
UCLASS(ClassGroup=(TD), meta=(BlueprintSpawnableComponent))
class TOWERDEFENCE_API UTDPoolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDPoolComponent();

	/**
	 * 풀에서 Actor를 꺼내거나 새로 스폰.
	 * ITDPoolActorInterface를 구현하지 않은 클래스는 거부됨.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|Pool")
	AActor* GetPoolActorFromClass(TSubclassOf<AActor> ActorClass, FTransform Transform, AActor* NewOwner);

	/**
	 * Actor를 풀에 반납. OnAddedToPool() 호출됨 (Hidden + Tick 비활성화).
	 * ITDPoolActorInterface를 구현하지 않은 Actor는 무시됨.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|Pool")
	void ReturnToPool(AActor* PoolActor);

private:
	TMap<TSubclassOf<AActor>, TArray<AActor*>> ActorPool;
};
