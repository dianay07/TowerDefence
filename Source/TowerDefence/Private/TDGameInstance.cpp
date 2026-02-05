#include "TDGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "TDMainSaveGame.h"

const FString MainSaveSlotName = TEXT("MainSave");

UTDGameInstance::UTDGameInstance()
{
	SaveSlotIndex = 0;
	SaveSlotNames.Add(0, TEXT("SaveSlotOne"));
	SaveSlotNames.Add(1, TEXT("SaveSlotTwo"));
	SaveSlotNames.Add(2, TEXT("SaveSlotThree"));
}

void UTDGameInstance::Init()
{
	LoadMain();

	Super::Init();
}

int32 UTDGameInstance::GetSaveSlot()
{
	return SaveSlotIndex;
}

FString UTDGameInstance::GetSaveSlotName(int32 Index) const
{
	return SaveSlotNames.FindRef(Index);
}

void UTDGameInstance::SetSaveSlot(int32 Index)
{
	SaveSlotIndex = Index;
	SaveMain();
}

void UTDGameInstance::LoadMain()
{
	UTDMainSaveGame* MainSaveGame = Cast<UTDMainSaveGame>(UGameplayStatics::LoadGameFromSlot(MainSaveSlotName, 0));
	if (MainSaveGame)
	{
		SaveSlotIndex = MainSaveGame->SaveSlotIndex;
	}
}

void UTDGameInstance::SaveMain()
{
	UTDMainSaveGame* MainSaveGame = Cast<UTDMainSaveGame>(UGameplayStatics::CreateSaveGameObject(UTDMainSaveGame::StaticClass()));
	if (!MainSaveGame)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create MainSaveGame"));
	}

	MainSaveGame->SaveSlotIndex = SaveSlotIndex;
	if(!UGameplayStatics::SaveGameToSlot(MainSaveGame, MainSaveSlotName, 0))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save MainSaveGame"));
	}

	LoadMain();
}