#pragma once

#include "CoreMinimal.h"
#include "GAS/TDBaseSet.h"
#include "AbilitySystemComponent.h"
#include "TDEnemySet.generated.h"

UCLASS()
class TOWERDEFENCE_API UTDEnemySet : public UTDBaseSet
{
	GENERATED_BODY()
	
public:
	UTDEnemySet();

	UPROPERTY(BlueprintReadOnly, Category="Attributes", Meta=(AllowPrivateAccess = true))
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UTDEnemySet, Health);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UTDEnemySet, MaxHealth);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS_BASIC(UTDEnemySet, MoveSpeed);
	
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS_BASIC(UTDEnemySet, Damage);
	
protected:
	virtual void ClampAttributeOnChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
};
