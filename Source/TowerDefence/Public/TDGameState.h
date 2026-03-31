#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TDGameState.generated.h"

UCLASS()
class TOWERDEFENCE_API ATDGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ATDGameState();

	// Phase 2에서 ReplicatedUsing=OnRep_* 으로 교체 예정
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	int32 SharedGold = 200;

	UPROPERTY(BlueprintReadOnly, Category = "Game")
	int32 BaseHealth = 20;

	UPROPERTY(BlueprintReadOnly, Category = "Game")
	int32 CurrentWave = 0;
};
