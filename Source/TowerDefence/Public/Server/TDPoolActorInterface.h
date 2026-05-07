#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "TDPoolActorInterface.generated.h"

UINTERFACE(MinimalAPI)
class UTDPoolActorInterface : public UInterface
{
	GENERATED_BODY()
};

class TOWERDEFENCE_API ITDPoolActorInterface
{
	GENERATED_BODY()

public:
	
	virtual void OnAddedToPool() = 0;
	virtual void OnRemovedFromPool() = 0;
};
