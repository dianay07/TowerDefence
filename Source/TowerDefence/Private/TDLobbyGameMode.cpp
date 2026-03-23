#include "TDLobbyGameMode.h"
#include "TDLobbyGameState.h"
#include "TDGameInstance.h"
#include "GameFramework/PlayerState.h"

ATDLobbyGameMode::ATDLobbyGameMode()
{
	// LobbyGameState를 기본 GameState 클래스로 설정
	GameStateClass = ATDLobbyGameState::StaticClass();
}

void ATDLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer) return;

	ATDLobbyGameState* LobbyGS = GetGameState<ATDLobbyGameState>();
	if (!LobbyGS) return;

	// PlayerState가 준비될 때까지 잠시 대기 후 이름 등록
	// PostLogin 시점에는 PlayerState가 이미 할당되어 있음
	if (APlayerState* PS = NewPlayer->GetPlayerState<APlayerState>())
	{
		FString PlayerName = PS->GetPlayerName();
		if (PlayerName.IsEmpty())
		{
			PlayerName = FString::Printf(TEXT("Player%d"), LobbyGS->LobbyPlayerNames.Num() + 1);
		}
		LobbyGS->AddLobbyPlayer(PlayerName);
		UE_LOG(LogTemp, Log, TEXT("[LobbyGameMode] PostLogin: %s 참가"), *PlayerName);
	}
}

void ATDLobbyGameMode::Logout(AController* Exiting)
{
	if (Exiting)
	{
		ATDLobbyGameState* LobbyGS = GetGameState<ATDLobbyGameState>();
		if (LobbyGS)
		{
			if (APlayerController* PC = Cast<APlayerController>(Exiting))
			{
				if (APlayerState* PS = PC->GetPlayerState<APlayerState>())
				{
					FString PlayerName = PS->GetPlayerName();
					LobbyGS->RemoveLobbyPlayer(PlayerName);
					UE_LOG(LogTemp, Log, TEXT("[LobbyGameMode] Logout: %s 퇴장"), *PlayerName);
				}
			}
		}
	}

	Super::Logout(Exiting);
}

void ATDLobbyGameMode::StartLobbyGame()
{
	UTDGameInstance* GI = GetGameInstance<UTDGameInstance>();
	if (!GI) return;
	GI->ServerTravelToGameMap();
}
