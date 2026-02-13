#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TDPoolActor.h"
#include "TDPooledGameMode.generated.h"


UCLASS()
class TOWERDEFENCE_API ATDPooledGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
	TMap<TSubclassOf<ATDPoolActor>, TArray<ATDPoolActor*>> ActorPool;
	
public:
	UFUNCTION(BlueprintCallable)
	AActor* GetPoolActorFromClass(TSubclassOf<ATDPoolActor> TDPoolActorClass, FTransform Transform, AActor* NewOwner);
	
	UFUNCTION(BlueprintCallable)
	void PoolActor(ATDPoolActor* TDPoolActor);
};
