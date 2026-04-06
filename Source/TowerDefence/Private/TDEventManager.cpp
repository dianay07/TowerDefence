#include "TDEventManager.h"
#include "TDEnemyActor.h"

void UTDEventManager::BroadcastEnemyDied(ATDEnemyActor* Enemy)
{
	OnEnemyDied.Broadcast(Enemy);
}

void UTDEventManager::BroadcastCoinsChanged(int32 Change, int32 Coins)
{
	OnCoinsChanged.Broadcast(Change, Coins);
}

void UTDEventManager::BroadcastEnemyAttacked(ATDEnemyActor* Enemy)
{
	OnEnemyAttacked.Broadcast(Enemy);
}
