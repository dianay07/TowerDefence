#include "Lobby/TDLobbyPlayerController.h"
#include "Lobby/TDLobbyPlayerState.h"
#include "Lobby/TDLobbyGameState.h"
#include "Session/TDLobbySessionSubsystem.h"
#include "Kismet/GameplayStatics.h"

// ── 생명주기 ──────────────────────────────────────────────────────────────────

void ATDLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 마우스 커서 항상 표시 (로비 UI 조작용)
	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());
}

// ── 로비 액션 (Client → Server RPC) ──────────────────────────────────────────

void ATDLobbyPlayerController::Server_SetReady_Implementation(bool bReady)
{
	if (ATDLobbyPlayerState* PS = GetPlayerState<ATDLobbyPlayerState>())
	{
		PS->SetReady(bReady);
	}
}

void ATDLobbyPlayerController::Server_RequestStart_Implementation()
{
	// 호스트 게이트: 서버 PC 가 아니면(즉 클라이언트가 호출 시) 무시
	// Listen Server 에서 호스트 본인 PC 는 NetConnection 이 nullptr
	if (GetNetConnection() != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyPC] Server_RequestStart: 호스트만 시작 가능."));
		return;
	}

	ATDLobbyGameState* GS = GetWorld()->GetGameState<ATDLobbyGameState>();
	if (!GS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyPC] Server_RequestStart: LobbyGameState 없음."));
		return;
	}

	// (선택) 전원 준비 완료 체크 — 필요 없으면 주석 해제
	// if (!GS->AreAllPlayersReady()) { return; }

	const FString TravelURL = GameLevelPath + TEXT("?listen");
	GetWorld()->ServerTravel(TravelURL, true);

	UE_LOG(LogTemp, Log, TEXT("[LobbyPC] 게임 시작 → ServerTravel: %s"), *TravelURL);
}

// ── 서버 → 클라 명령 ─────────────────────────────────────────────────────────

void ATDLobbyPlayerController::Client_ReturnToMainMenu_Implementation(const FString& Reason)
{
	UE_LOG(LogTemp, Log, TEXT("[LobbyPC] 메인 메뉴로 복귀: %s"), *Reason);

	// 세션 정리 후 메인 메뉴로 이동
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTDLobbySessionSubsystem* Sub = GI->GetSubsystem<UTDLobbySessionSubsystem>())
		{
			Sub->DestroySession();
		}
	}

	// 메인 메뉴 레벨로 클라이언트 트래블
	ClientTravel(TEXT("/Game/MainMenu/SplashLevel"), ETravelType::TRAVEL_Absolute);
}
