#include "Lobby/TDLobbyGameState.h"
#include "Lobby/TDLobbyPlayerState.h"
#include "Net/UnrealNetwork.h"

// ── 생명주기 ──────────────────────────────────────────────────────────────────

void ATDLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATDLobbyGameState, PlayerSlots);
}

// ── 참가자 슬롯 ───────────────────────────────────────────────────────────────

void ATDLobbyGameState::AddPlayer(ATDLobbyPlayerState* PS)
{
	if (!HasAuthority() || !IsValid(PS)) return;
	if (PlayerSlots.Contains(PS)) return;

	PlayerSlots.Add(PS);
	OnPlayerSlotsChanged.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("[LobbyGameState] 참가자 추가: %s (총 %d 명)"),
		*PS->GetPlayerName(), PlayerSlots.Num());
}

void ATDLobbyGameState::RemovePlayer(ATDLobbyPlayerState* PS)
{
	if (!HasAuthority() || !IsValid(PS)) return;

	PlayerSlots.Remove(PS);
	OnPlayerSlotsChanged.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("[LobbyGameState] 참가자 제거: %s (총 %d 명)"),
		*PS->GetPlayerName(), PlayerSlots.Num());
}

bool ATDLobbyGameState::AreAllPlayersReady() const
{
	if (PlayerSlots.IsEmpty()) return false;

	for (const TObjectPtr<ATDLobbyPlayerState>& PS : PlayerSlots)
	{
		if (!IsValid(PS) || !PS->bIsReady) return false;
	}
	return true;
}

void ATDLobbyGameState::OnRep_PlayerSlots()
{
	// 클라에서 복제 수신 → 위젯 갱신
	OnPlayerSlotsChanged.Broadcast();
}
