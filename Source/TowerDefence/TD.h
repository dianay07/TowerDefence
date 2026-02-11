#pragma once

#include "CoreMinimal.h"
#include "TD.generated.h"

UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	None,
	Green,
	Purple,
	Red,
	Yellow
};

UENUM(BlueprintType)
enum class ETowerActions : uint8
{
	None,
	BuildTurret,
	BuildBallista,
	BuildCatapult,
	BuildCannon,
	Upgrade,
	BreakDown
};

USTRUCT(BlueprintType)
struct FWaveData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StartTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EEnemyType EnemyType = EEnemyType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SpawnCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpawnDeley = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DelayRemaining = 0.f;
};

UENUM(BlueprintType)
enum class ETowerType : uint8
{
	None,
	Turret,
	Ballista,
	Catapult,
	Cannon,
	
};


USTRUCT(BlueprintType)
struct FTowerData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETowerType EnemyType = ETowerType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TowerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BuildCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 UpgradeCost1 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 UpgradeCost2 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 UpgradeCost3 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Range = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FireRate = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Radius = 0.f;
};

UENUM(BlueprintType)
enum class EQualityOptionType : uint8
{
	None,
	Overall,
	ViewDistance,
	Shadow,
	GlobalIllumination,
	Reflections,
	AntiAliasing,
	Texture,
	VisualEffect,
	PostProcessing,
	Foliage,
	Shading
};