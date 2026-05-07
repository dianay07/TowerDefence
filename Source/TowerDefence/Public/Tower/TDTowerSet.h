#pragma once

#include "CoreMinimal.h"
#include "GAS/TDBaseSet.h"
#include "AbilitySystemComponent.h"
#include "TDTowerSet.generated.h"


UCLASS()
class TOWERDEFENCE_API UTDTowerSet : public UTDBaseSet
{
	GENERATED_BODY()
	
public:
	UTDTowerSet();

	UPROPERTY(BlueprintReadOnly, Category="Attributes", Meta=(AllowPrivateAcess = true))
	FGameplayAttributeData Range;
	ATTRIBUTE_ACCESSORS_BASIC(UTDTowerSet, Range);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", Meta = (AllowPrivateAcess = true))
	FGameplayAttributeData FireRate;
	ATTRIBUTE_ACCESSORS_BASIC(UTDTowerSet, FireRate);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", Meta = (AllowPrivateAcess = true))
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS_BASIC(UTDTowerSet, Damage);
};
