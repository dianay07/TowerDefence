#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TDLobbyGameMode.generated.h"

UCLASS()
class TOWERDEFENCE_API ATDLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATDLobbyGameMode();

	// 플레이어 참가 시 호출 (서버 전용)
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// 플레이어 퇴장 시 호출 (서버 전용)
	virtual void Logout(AController* Exiting) override;

	// Blueprint에서 호출 – 서버에서만 실행됨 (GameMode는 서버 전용)
	UFUNCTION(BlueprintCallable, Category="Lobby")
	void StartLobbyGame();
};
