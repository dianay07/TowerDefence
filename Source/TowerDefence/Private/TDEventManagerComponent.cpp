#include "TDEventManagerComponent.h"
#include "TDEnemyActor.h"

// ── 적 사망 이벤트 ────────────────────────────────────────────────────────────

void UTDEventManagerComponent::BroadcastEnemyDied(ATDEnemyActor* Enemy)
{
	OnEnemyDied.Broadcast(Enemy);
}

// ── 적 공격 이벤트 ────────────────────────────────────────────────────────────

void UTDEventManagerComponent::BroadcastEnemyAttacked(ATDEnemyActor* Enemy)
{
	OnEnemyAttacked.Broadcast(Enemy);
}
