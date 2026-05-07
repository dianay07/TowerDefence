#include "TDGameState.h"
#include "Tower/TDTowerBase.h"
#include "Enemy/TDEnemyActor.h"
#include "Net/UnrealNetwork.h"

ATDGameState::ATDGameState()
{
	// 초기값은 헤더 선언부에서 설정
}

void ATDGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATDGameState, SharedCoin);
	DOREPLIFETIME(ATDGameState, BaseHealth);
	DOREPLIFETIME(ATDGameState, MaxBaseHealth);
	DOREPLIFETIME(ATDGameState, CurrentWave);
	DOREPLIFETIME(ATDGameState, PlacedTowers);
	DOREPLIFETIME(ATDGameState, ActiveEnemies);
}

// ── 복제 콜백 ─────────────────────────────────────────────────────────────────
void ATDGameState::OnRep_SharedCoin()
{
	OnCoinsChanged.Broadcast(0, SharedCoin);
}

void ATDGameState::OnRep_BaseHealth()
{
	OnBaseHealthChanged.Broadcast(BaseHealth, MaxBaseHealth);
}

void ATDGameState::OnRep_CurrentWave()
{
	// 웨이브 UI 업데이트 delegate 연결 시 여기서 broadcast
}

// ── 기지 체력 ─────────────────────────────────────────────────────────────────
void ATDGameState::DecreaseBaseHealth()
{
	if (!HasAuthority()) return;
	BaseHealth = FMath::Max(0, BaseHealth - 1);
	OnBaseHealthChanged.Broadcast(BaseHealth, MaxBaseHealth);
}

// ── 코인 ──────────────────────────────────────────────────────────────────────
int32 ATDGameState::GetCoins() const
{
	return SharedCoin;
}

bool ATDGameState::HasCoins(int32 Amount) const
{
	return GetCoins() >= Amount;
}

void ATDGameState::CoinChange(int32 change)
{
	if (!HasAuthority()) return;
	SharedCoin = FMath::Max(0, SharedCoin + change);
	OnCoinsChanged.Broadcast(change, SharedCoin);
}

// ── 배치된 타워 ───────────────────────────────────────────────────────────────
void ATDGameState::RegisterTower(ATDTowerBase* Tower)
{
	if (!HasAuthority() || !IsValid(Tower)) return;
	PlacedTowers.AddUnique(Tower);
}

void ATDGameState::UnregisterTower(ATDTowerBase* Tower)
{
	if (!HasAuthority()) return;
	PlacedTowers.Remove(Tower);
}

void ATDGameState::OnRep_PlacedTowers()
{
	// 클라이언트 측 타워 목록 갱신 시 필요한 UI 업데이트 등 처리
}

// ── 활성 적 목록 ──────────────────────────────────────────────────────────────
void ATDGameState::RegisterEnemy(ATDEnemyActor* Enemy)
{
	if (!HasAuthority() || !IsValid(Enemy)) return;
	ActiveEnemies.AddUnique(Enemy);
}

void ATDGameState::UnregisterEnemy(ATDEnemyActor* Enemy)
{
	if (!HasAuthority()) return;
	ActiveEnemies.Remove(Enemy);
}

void ATDGameState::OnRep_ActiveEnemies()
{
	// 클라이언트 측 적 목록 갱신 — UI/미니맵 등 연결 시 여기서 broadcast
}

ATDEnemyActor* ATDGameState::GetFurthestEnemy(FVector Location, float Radius) const
{
	float HighDist  = 0.f;
	ATDEnemyActor* HighEnemy = nullptr;

	for (ATDEnemyActor* Enemy : ActiveEnemies)
	{
		if (!IsValid(Enemy)) continue;
		if (FVector::Dist(Enemy->GetActorLocation(), Location) <= Radius)
		{
			if (Enemy->GetDistance() > HighDist)
			{
				HighDist  = Enemy->GetDistance();
				HighEnemy = Enemy;
			}
		}
	}

	return HighEnemy;
}

TArray<ATDEnemyActor*> ATDGameState::GetEnemiesInRange(FVector Location, float Radius) const
{
	TArray<ATDEnemyActor*> Result;

	for (ATDEnemyActor* Enemy : ActiveEnemies)
	{
		if (!IsValid(Enemy)) continue;
		if (FVector::Dist(Enemy->GetActorLocation(), Location) <= Radius)
		{
			Result.Add(Enemy);
		}
	}

	return Result;
}

// ── 적 이벤트 Multicast RPC ───────────────────────────────────────────────────
void ATDGameState::NotifyEnemyDied(ATDEnemyActor* Enemy)
{
	if (!HasAuthority()) return;
	Multicast_OnEnemyDied(Enemy);
}

void ATDGameState::NotifyEnemyAttacked(ATDEnemyActor* Enemy, float Damage)
{
	if (!HasAuthority()) return;
	Multicast_OnEnemyAttacked(Enemy, Damage);
}

void ATDGameState::Multicast_OnEnemyDied_Implementation(ATDEnemyActor* Enemy)
{
	OnEnemyDied.Broadcast(Enemy);
}

void ATDGameState::Multicast_OnEnemyAttacked_Implementation(ATDEnemyActor* Enemy, float Damage)
{
	OnEnemyAttacked.Broadcast(Enemy, Damage);
}

// ── 게임 종료 ─────────────────────────────────────────────────────────────────

void ATDGameState::BroadcastGameEnded(bool bWin)
{
	OnGameEnded.Broadcast(bWin);
}
