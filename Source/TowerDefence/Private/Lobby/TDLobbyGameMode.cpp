#include "Lobby/TDLobbyGameMode.h"
#include "Lobby/TDLobbyGameState.h"
#include "Lobby/TDLobbyPlayerState.h"
#include "Lobby/TDLobbyPlayerController.h"

// ── 생명주기 ──────────────────────────────────────────────────────────────────

ATDLobbyGameMode::ATDLobbyGameMode()
{
	// 로비 전용 클래스 등록
	PlayerControllerClass = ATDLobbyPlayerController::StaticClass();
	GameStateClass        = ATDLobbyGameState::StaticClass();
	PlayerStateClass      = ATDLobbyPlayerState::StaticClass();
}

void ATDLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// Listen Server 에서 호스트 본인의 PC 도 PostLogin 을 탄다 → 동일하게 슬롯 추가
	if (ATDLobbyGameState* GS = GetGameState<ATDLobbyGameState>())
	{
		if (ATDLobbyPlayerState* PS = NewPlayer->GetPlayerState<ATDLobbyPlayerState>())
		{
			GS->AddPlayer(PS);
		}
	}
}

void ATDLobbyGameMode::Logout(AController* Exiting)
{
	ATDLobbyGameState* GS = GetGameState<ATDLobbyGameState>();
	APlayerController* ExitingPC = Cast<APlayerController>(Exiting);

	if (GS && ExitingPC)
	{
		if (ATDLobbyPlayerState* PS = ExitingPC->GetPlayerState<ATDLobbyPlayerState>())
		{
			GS->RemovePlayer(PS);
		}

		// 호스트 퇴장 감지: NetConnection == nullptr 이 호스트 PC
		if (ExitingPC->GetNetConnection() == nullptr)
		{
			UE_LOG(LogTemp, Log, TEXT("[LobbyGameMode] 호스트가 나갔습니다. 모든 클라이언트에 복귀 명령."));
			NotifyAllClientsHostLeft();
		}
	}

	Super::Logout(Exiting);
}

// ── 호스트 종료 처리 ─────────────────────────────────────────────────────────

void ATDLobbyGameMode::NotifyAllClientsHostLeft()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ATDLobbyPlayerController* PC = Cast<ATDLobbyPlayerController>(It->Get());
		// 호스트 본인 (NetConnection == nullptr) 은 건너뜀
		if (PC && PC->GetNetConnection() != nullptr)
		{
			PC->Client_ReturnToMainMenu(TEXT("호스트가 나갔습니다."));
		}
	}
}
