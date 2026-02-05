#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TDMainSaveGame.generated.h"


UCLASS()
class TOWERDEFENCE_API UTDMainSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	UTDMainSaveGame();

	UPROPERTY(VisibleAnywhere)
	int32 SaveSlotIndex;
};
