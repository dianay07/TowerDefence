#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpec.h"
#include "TDEnemyActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDied);

UCLASS()
class TOWERDEFENCE_API ATDEnemyActor : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* StaticMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneRootComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	class UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	class UTDEnemySet* EnemySet;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> DefaultEffect;

protected:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stats")
	float InitialHealth = 100.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stats")
	float InitialMoveSpeed = 300.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stats")
	float InitialDamage = 10.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stats")
	int32 RewardCoin = 100;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool IsDead = false;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDied OnDied;

public:
	ATDEnemyActor();

protected:
	virtual void BeginPlay() override;

	UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAS")
	void OnHealthChanged(float OldValue, float NewValue);

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void PlayDeathAnimation();

	void InitializeASC();
	void OnEnemyDied();

public:
	virtual void PostInitializeComponents() override;

	virtual void Tick(float DeltaTime) override;
};
