#include "TDLobbyGameState.h"
#include "Net/UnrealNetwork.h"

ATDLobbyGameState::ATDLobbyGameState()
{
}

void ATDLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATDLobbyGameState, LobbyPlayerNames);
}

void ATDLobbyGameState::AddLobbyPlayer(const FString& PlayerName)
{
	if (!HasAuthority()) return;

	LobbyPlayerNames.AddUnique(PlayerName);

	// 서버에서도 델리게이트 브로드캐스트 (OnRep은 클라이언트에서만 호출됨)
	OnLobbyPlayersUpdated.Broadcast();
}

void ATDLobbyGameState::RemoveLobbyPlayer(const FString& PlayerName)
{
	if (!HasAuthority()) return;

	LobbyPlayerNames.Remove(PlayerName);

	// 서버에서도 델리게이트 브로드캐스트
	OnLobbyPlayersUpdated.Broadcast();
}

void ATDLobbyGameState::OnRep_LobbyPlayers()
{
	// 클라이언트에서 복제 수신 시 호출됨
	OnLobbyPlayersUpdated.Broadcast();
}
