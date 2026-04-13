#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpec.h"
#include "TDEnemyActor.generated.h"

class ATDPath;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDied, ATDEnemyActor*, Enemy);

UCLASS()
class TOWERDEFENCE_API ATDEnemyActor : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ATDEnemyActor();

	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;

// ── 컴포넌트 ──────────────────────────────────────────────────────────────────
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneRootComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* StaticMeshComp;

// ── GAS & 스탯 초기화 ─────────────────────────────────────────────────────────
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	class UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	class UTDEnemySet* EnemySet;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> DefaultEffect;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stats")
	float InitialHealth = 100.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stats")
	float InitialMoveSpeed = 300.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stats")
	float InitialDamage = 10.f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Stats")
	int32 RewardCoin = 100;

protected:
	virtual void BeginPlay() override;
	UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	void InitializeASC();
	virtual void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAS")
	void OnHealthChanged(float OldValue, float NewValue);

// ── 경로 이동 ─────────────────────────────────────────────────────────────────
protected:
	UPROPERTY()
	ATDPath* CurrentPath;

	UPROPERTY(BlueprintReadOnly, Category = "Path")
	float Distance = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool IsDead = false;

public:
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy")
	void InitializePath(ATDPath* Path);

	UFUNCTION(BlueprintCallable, Category = "TD|Enemy")
	float Advance(float DeltaTime);

	FORCEINLINE float GetDistance() const { return Distance; }

// ── 사망 이벤트 ───────────────────────────────────────────────────────────────
protected:
	void OnEnemyDied();

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void PlayDeathAnimation();

public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDied OnDied;
};
