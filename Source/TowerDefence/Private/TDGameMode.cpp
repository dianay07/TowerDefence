#include "TDGameMode.h"
#include "TDGameInstance.h"
#include "TDGameState.h"
#include "Kismet/GameplayStatics.h"
#include "TDFL_Utility.h"

ATDGameMode::ATDGameMode()
{
	EventManager = CreateDefaultSubobject<UTDEventManagerComponent>(TEXT("EventManager"));
	WaveManager  = CreateDefaultSubobject<UTDWaveManagerComponent>(TEXT("WaveManager"));
}

void ATDGameMode::BeginPlay()
{
	Super::BeginPlay();

}

// ── Game State ────────────────────────────────────────────────────────────────

void ATDGameMode::GameEnded(bool bWin)
{
	UGameplayStatics::SetGamePaused(this, true);

	if (bWin)
	{
		// TODO: UTDGameInstance::OnLevelComplete() 포팅 필요
		UTDGameInstance* GI = Cast<UTDGameInstance>(GetGameInstance());
		// if (GI) GI->OnLevelComplete();
	}
}

void ATDGameMode::CheckIfWin()
{
	if (WaveManager->DoEnemiesRemain())
		GameEnded(true);
	else
		GameEnded(false);
}

void ATDGameMode::CheckIfLoss()
{
	if (UTDFL_Utility::GetTDGameState(this)->BaseHealth <= 0)
	{
		GameEnded(false);
	}
}

// ── Pool ──────────────────────────────────────────────────────────────────────
AActor* ATDGameMode::GetPoolActorFromClass(TSubclassOf<AActor> ActorClass, FTransform Transform, AActor* NewOwner)
{
	if (!ActorClass || !ActorClass->ImplementsInterface(UTDPoolActorInterface::StaticClass()))
	{
		return nullptr;
	}

	AActor* PoolActor = nullptr;

	if (ActorPool.Contains(ActorClass))
	{
		TArray<AActor*>* PoolActorArray = ActorPool.Find(ActorClass);

		if (PoolActorArray && !PoolActorArray->IsEmpty())
		{
			PoolActor = PoolActorArray->Pop();
		}
	}

	if (!PoolActor)
		PoolActor = Cast<AActor>(GetWorld()->SpawnActor<AActor>(ActorClass));

	if (PoolActor)
	{
		PoolActor->SetActorTransform(Transform);
		PoolActor->SetOwner(NewOwner);
		ITDPoolActorInterface* PoolActorInter = Cast<ITDPoolActorInterface>(PoolActor);
		if (PoolActorInter)
		{
			PoolActorInter->OnRemovedFromPool();
		}
	}

	return PoolActor;
}

void ATDGameMode::PoolActor(AActor* PoolActor)
{
	if (!PoolActor || !Cast<ITDPoolActorInterface>(PoolActor))
	{
		return;
	}

	TSubclassOf<AActor> ActorClass = PoolActor->GetClass();
	if (ActorPool.Contains(ActorClass))
	{
		TArray<AActor*>* PoolActorArray = ActorPool.Find(ActorClass);

		if (PoolActorArray)
		{
			PoolActorArray->Add(PoolActor);
		}
	}
	else
	{
		TArray<AActor*> PoolActorArray;
		PoolActorArray.Add(PoolActor);
		ActorPool.Add(ActorClass, PoolActorArray);
	}

	Cast<ITDPoolActorInterface>(PoolActor)->OnAddedToPool();
}
