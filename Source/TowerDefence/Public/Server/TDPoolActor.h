#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Server/TDPoolActorInterface.h"
#include "TDPoolActor.generated.h"

UCLASS()
class TOWERDEFENCE_API ATDPoolActor : public AActor, public ITDPoolActorInterface
{
	GENERATED_BODY()
	
public:	
	ATDPoolActor();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnPoolSpawned();
	
	virtual void OnAddedToPool();
	virtual void OnRemovedFromPool();
};
