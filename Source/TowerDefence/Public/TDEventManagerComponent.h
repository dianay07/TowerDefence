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

// ── 적 사망 이벤트 ────────────────────────────────────────────────────────────
public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEnemyDied OnEnemyDied;

	UFUNCTION(BlueprintCallable, Category = "Events")
	void BroadcastEnemyDied(ATDEnemyActor* Enemy);

// ── 적 공격 이벤트 ────────────────────────────────────────────────────────────
public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnEnemyAttacked OnEnemyAttacked;

	UFUNCTION(BlueprintCallable, Category = "Events")
	void BroadcastEnemyAttacked(ATDEnemyActor* Enemy);
};
