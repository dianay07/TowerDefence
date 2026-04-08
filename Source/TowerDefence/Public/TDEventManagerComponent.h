#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TDEventManagerComponent.generated.h"

class ATDEnemyActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDied,     ATDEnemyActor*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyAttacked, ATDEnemyActor*, Enemy);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOWERDEFENCE_API UTDEventManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEnemyDied OnEnemyDied;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEnemyAttacked OnEnemyAttacked;

public:
	UFUNCTION(BlueprintCallable, Category = "Events")
	void BroadcastEnemyDied(ATDEnemyActor* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Events")
	void BroadcastEnemyAttacked(ATDEnemyActor* Enemy);
};
