#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpec.h"
#include "TDEnemyActor.generated.h"

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

public:
	ATDEnemyActor();

protected:
	virtual void BeginPlay() override;

	UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAS")
	void OnHealthChanged(float OldValue, float NewValue);

	void InitializeASC();

public:
	virtual void PostInitializeComponents() override;

	virtual void Tick(float DeltaTime) override;
};
