// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TDPoolActor.h"
#include "GameplayEffect.h"

#include "TDProjectile.generated.h"

class ATDEnemyActor;

UCLASS()
class TOWERDEFENCE_API ATDProjectile : public ATDPoolActor
{
	GENERATED_BODY()

public:
	ATDProjectile();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// ── Components ───────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	// ── Variables ────────────────────────────────────────────────
	UPROPERTY(BlueprintReadWrite, Category = "Projectile")
	TObjectPtr<ATDEnemyActor> Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float LastDistance = 99999.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Speed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Radius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	TSubclassOf<UGameplayEffect> BP_GE_DamageClass;

	// ── Functions ────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void MoveTowardsTarget(float Delta);

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void OnHitTarget();

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SetProjectileData(ATDEnemyActor* InTarget, float InDamage, float InRadius);

	virtual void OnAddedToPool();
	virtual void OnRemovedFromPool();

private:
	/** 비행 종료 시 정리 — 서버는 풀 반납, 클라이언트는 즉시 Destroy. */
	void DespawnSelf();
};
