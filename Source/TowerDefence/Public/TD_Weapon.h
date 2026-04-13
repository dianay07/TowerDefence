// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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
	TSubclassOf<AActor> ProjectileClass;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<ATDTowerBase> Tower;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	TArray<TObjectPtr<USceneComponent>> FirePoints;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	int32 FirePointIndex = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<AActor> Projectile;

	// ── Functions ────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FindEnemy();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FaceEnemy();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void FireAtEnemy();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void GetFirePoint(FVector& OutLocation, FRotator& OutRotation);
};
