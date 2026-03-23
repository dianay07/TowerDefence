#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TDLobbyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyPlayersUpdated);

UCLASS()
class TOWERDEFENCE_API ATDLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ATDLobbyGameState();

	// 서버에서 복제되는 플레이어 이름 목록
	UPROPERTY(ReplicatedUsing=OnRep_LobbyPlayers, BlueprintReadOnly, Category="Lobby")
	TArray<FString> LobbyPlayerNames;

	// 목록 변경 시 Blueprint에서 바인딩할 델리게이트
	UPROPERTY(BlueprintAssignable, Category="Lobby")
	FOnLobbyPlayersUpdated OnLobbyPlayersUpdated;

	// 서버에서 호출 - 플레이어 추가
	void AddLobbyPlayer(const FString& PlayerName);

	// 서버에서 호출 - 플레이어 제거
	void RemoveLobbyPlayer(const FString& PlayerName);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_LobbyPlayers();
};
