#include "TDGameState.h"

ATDGameState::ATDGameState()
{
	SharedCoin = 2000;
	BaseHealth = 20;
	MaxBaseHealth = 20;
	CurrentWave = 0;
}

void ATDGameState::DecreaseBaseHealth()
{
	// Phase 2: if (!HasAuthority()) return;
	BaseHealth = FMath::Max(0, BaseHealth - 1);
	OnBaseHealthChanged.Broadcast(BaseHealth, MaxBaseHealth);
	// Phase 2: Broadcast를 OnRep_BaseHealth()로 이전
}

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
	// Phase 2: if (!HasAuthority()) return;
	SharedCoin = FMath::Max(0, SharedCoin + change);
	OnSharedCoinChanged.Broadcast(change, SharedCoin);
	// Phase 2: Broadcast를 OnRep_SharedCoin()로 이전
}