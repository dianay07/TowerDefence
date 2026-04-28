	// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/NetSerialization.h"
#include "TDProjectile.h"

#include "TD_Weapon.generated.h"

class ATDEnemyActor;
class ATDTowerBase;

UCLASS()
class TOWERDEFENCE_API ATD_Weapon : public AActor
{
	GENERATED_BODY()

public:
	ATD_Weapon();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// ── Components ───────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> Bottom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> Top;

	// ── Variables ────────────────────────────────────────────────
	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<ATDEnemyActor> Target;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<ATDProjectile> ProjectileClass;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<ATDTowerBase> Tower;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	TArray<TObjectPtr<USceneComponent>> FirePoints;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	int32 FirePointIndex = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<ATDProjectile> Projectile;

	// ── Functions ────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FindEnemy();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FaceEnemy();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FireAtEnemy();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void GetFirePoint(FVector& OutLocation, FRotator& OutRotation);

	/**
	 * 모든 머신(서버+클라)에 동시 통보 — 각자 자기 로컬 Projectile 스폰.
	 * Projectile 자체는 비복제(코스메틱). 데미지는 서버 권위로 적용.
	 */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFireProjectile(ATDEnemyActor* InTarget,
	                             float InDamage, float InRadius,
	                             FVector_NetQuantize SpawnLocation,
	                             FRotator SpawnRotation);
};
