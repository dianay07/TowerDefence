#include "TDEnemySet.h"

UTDEnemySet::UTDEnemySet()
	: Health(10.0f), MaxHealth(10.0f), MoveSpeed(100.0f), Damage(10.0f)
{
}

void UTDEnemySet::ClampAttributeOnChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if(Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
}