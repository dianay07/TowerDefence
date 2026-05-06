// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TDTowerPawn.h"
#include "Components/BoxComponent.h"
#include "Components/ChildActorComponent.h"
#include "TowerDefence/TD.h"

#include "TDTowerBase.generated.h"

/**
 * 
 */
UCLASS()
class TOWERDEFENCE_API ATDTowerBase : public ATDTowerPawn
{
	GENERATED_BODY()
	
	ATDTowerBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* ChildActorWeaponComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* HighlightStaticMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* BoxComp;

    // Variables (Image 2)
    UPROPERTY(BlueprintReadWrite, Category = "TowerBase")
    TObjectPtr<AActor> BP_TowerActions; // BP 를 통해서 꼭 셋팅하게 해야 함.

    UPROPERTY(BlueprintReadWrite, Category = "TowerBase")
    TObjectPtr<AActor> Weapon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TowerBase")
    ETowerType Type;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TowerBase")
    FTowerData TowerData;

    UPROPERTY(ReplicatedUsing=OnRep_UpgradeLevel, EditAnywhere, BlueprintReadWrite, Category = "TowerBase")
    int32 UpgradeLevel = 0;

    UFUNCTION()
    void OnRep_UpgradeLevel();

    UPROPERTY(BlueprintReadWrite, Category = "TowerBase")
    bool IsDataValid = false;

    // BP에서 TowerActions 위젯 클래스 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TowerBase")
    TSubclassOf<AActor> BP_TowerActionsClass;

public:
    // UTDTowerSpawnerComponent::DoTowerAction 에서 스폰할 타워 클래스 (BP 디폴트에서 설정)
    UPROPERTY(EditDefaultsOnly, Category = "TowerBase|Classes")
    TSubclassOf<AActor> TurretClass;

    UPROPERTY(EditDefaultsOnly, Category = "TowerBase|Classes")
    TSubclassOf<AActor> BallistaClass;

    UPROPERTY(EditDefaultsOnly, Category = "TowerBase|Classes")
    TSubclassOf<AActor> CatapultClass;

    UPROPERTY(EditDefaultsOnly, Category = "TowerBase|Classes")
    TSubclassOf<AActor> CannonClass;

    UPROPERTY(EditDefaultsOnly, Category = "TowerBase|Classes")
    TSubclassOf<AActor> BaseTowerClass;  // BreakDown 시 되돌릴 빈 타워
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TowerBase")
	ETowerActions TowerActionTop = ETowerActions::BuildTurret;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TowerBase")
	ETowerActions TowerActionLeft = ETowerActions::BuildBallista;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TowerBase")
	ETowerActions TowerActionRight = ETowerActions::BuildCatapult;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TowerBase")
	ETowerActions TowerActionBottom = ETowerActions::BuildCannon;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	UFUNCTION(BlueprintCallable, Category = "TowerBase")
	void SetHighlight(bool IsHightlighted);

	// BP Tower Base의 CanUpgrade 함수 포팅
	// UpgradeLevel이 최대(3)보다 낮을 때 true 반환
	UFUNCTION(BlueprintCallable, Category = "TowerBase")
	bool CanUpgrade() const;

	UFUNCTION(BlueprintPure, Category = "TowerBase")
	float GetRange() const;

	UFUNCTION(BlueprintPure, Category = "TowerBase")
	float GetDamage() const;

	UFUNCTION(BlueprintPure, Category = "TowerBase")
	float GetRadius() const;

	UFUNCTION(BlueprintPure, Category = "TowerBase")
	int32 GetUpgradeCost() const;

	UFUNCTION(BlueprintPure, Category = "TowerBase")
	int32 GetBreakdownRefund() const;

	// BP Tower Base의 Upgrade 함수 포팅
	// CanUpgrade 확인 후 TowerActionsClass 액터를 스폰
	UFUNCTION(BlueprintCallable, Category = "TowerBase")
	void Select();

	UFUNCTION(BlueprintCallable, Category = "TowerBase")
	void UnSelect();

	// TowerAction 에 따른 비용/환급액과 설명 반환
	UFUNCTION(BlueprintCallable, Category = "TowerBase")
	void GetTowerDetails(ETowerActions TowerAction, int32& OutCostOrRefund, FString& OutDescription);

	// 타워 업그레이드 (UpgradeLevel 증가) — UTDTowerSpawnerComponent::DoTowerAction 에서 호출
	UFUNCTION(BlueprintCallable, Category = "TowerBase")
	void UpgradeTower();

	// TowerManager에서 TowerData를 조회하고 GAS 어트리뷰트에 적용
	UFUNCTION(BlueprintCallable, Category = "TowerBase")
	void GetTowerData();

private:
	// FetchTowerData Then 1: 조회 성공 시 GAS 어트리뷰트에 값 반영
	void SetTowerAttributes();
};
