#include "TDEventManagerComponent.h"
#include "TDEnemyActor.h"

void UTDEventManagerComponent::BroadcastEnemyDied(ATDEnemyActor* Enemy)
{
	OnEnemyDied.Broadcast(Enemy);
}

void UTDEventManagerComponent::BroadcastEnemyAttacked(ATDEnemyActor* Enemy)
{
	OnEnemyAttacked.Broadcast(Enemy);
}
