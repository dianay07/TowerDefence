#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TDPoolActor.h"
#include "TDEventManager.h"
#include "TDGameMode.generated.h"


UCLASS()
class TOWERDEFENCE_API ATDGameMode : public AGameModeBase
{
	GENERATED_BODY()

	TMap<TSubclassOf<AActor>, TArray<AActor*>> ActorPool;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Managers")
	UTDEventManager* EventManager;

	ATDGameMode();

public:
	UFUNCTION(BlueprintCallable)
	AActor* GetPoolActorFromClass(TSubclassOf<AActor> ActorClass, FTransform Transform, AActor* NewOwner);

	UFUNCTION(BlueprintCallable)
	void PoolActor(AActor* PoolActor);
};
