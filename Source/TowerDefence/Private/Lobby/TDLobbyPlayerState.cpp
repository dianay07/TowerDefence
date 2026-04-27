#include "Lobby/TDLobbyPlayerState.h"
#include "Net/UnrealNetwork.h"

// ── 생명주기 ──────────────────────────────────────────────────────────────────

void ATDLobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATDLobbyPlayerState, bIsReady);
}

// ── 준비 상태 ─────────────────────────────────────────────────────────────────

void ATDLobbyPlayerState::SetReady(bool bReady)
{
	if (!HasAuthority()) return;
	bIsReady = bReady;
	OnReadyChanged.Broadcast();
}

void ATDLobbyPlayerState::OnRep_bIsReady()
{
	// 클라에서 복제 수신 → 위젯 갱신
	OnReadyChanged.Broadcast();
}
