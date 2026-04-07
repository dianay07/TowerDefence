#include "TDEventManager.h"
#include "TDEnemyActor.h"

void UTDEventManager::BroadcastEnemyDied(ATDEnemyActor* Enemy)
{
	OnEnemyDied.Broadcast(Enemy);
}

void UTDEventManager::BroadcastEnemyAttacked(ATDEnemyActor* Enemy)
{
	OnEnemyAttacked.Broadcast(Enemy);
}
