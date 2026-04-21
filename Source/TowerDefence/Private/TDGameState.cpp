#include "TDGameState.h"
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

// ── 게임 종료 ─────────────────────────────────────────────────────────────────

void ATDGameState::BroadcastGameEnded(bool bWin)
{
	OnGameEnded.Broadcast(bWin);
}
