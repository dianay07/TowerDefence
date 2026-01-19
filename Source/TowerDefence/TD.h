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