#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TDPoolActor.h"
#include "TDObjectPooled.generated.h"


UCLASS()
class TOWERDEFENCE_API ATDObjectPooled : public AGameModeBase
{
	GENERATED_BODY()

	TMap<TSubclassOf<AActor>, TArray<AActor*>> ActorPool;

public:
	UFUNCTION(BlueprintCallable)
	AActor* GetPoolActorFromClass(TSubclassOf<AActor> ActorClass, FTransform Transform, AActor* NewOwner);

	UFUNCTION(BlueprintCallable)
	void PoolActor(AActor* PoolActor);
};
