#include "TDPooledGameMode.h"

AActor* ATDPooledGameMode::GetPoolActorFromClass(TSubclassOf<ATDPoolActor> TDPoolActorClass, FTransform Transform, AActor* NewOwner)
{
	if (!TDPoolActorClass)
	{
		return nullptr;
	}

	ATDPoolActor* PoolActor = nullptr;

	if (ActorPool.Contains(TDPoolActorClass))
	{
		TArray<ATDPoolActor*>* PoolActorArray = ActorPool.Find(TDPoolActorClass);

		if (PoolActorArray && !PoolActorArray->IsEmpty())
		{
			PoolActor = PoolActorArray->Pop();
		}
	}

	if (!PoolActor)
		PoolActor = Cast<ATDPoolActor>(GetWorld()->SpawnActor<AActor>(TDPoolActorClass));

	if(PoolActor)
	{
		PoolActor->SetActorTransform(Transform);
		PoolActor->SetOwner(NewOwner);
		PoolActor->OnRemovedFromPool();
	}

	return PoolActor;
}

void ATDPooledGameMode::PoolActor(ATDPoolActor* TDPoolActor)
{
	if(!TDPoolActor)
	{
		return;
	}

	TSubclassOf<ATDPoolActor> TDPoolActorClass = TDPoolActor->GetClass();
	if (ActorPool.Contains(TDPoolActorClass))
	{
		TArray<ATDPoolActor*>* PoolActorArray = ActorPool.Find(TDPoolActorClass);

		if (PoolActorArray)
		{
			PoolActorArray->Add(TDPoolActor);
		}
	}
	else
	{
		TArray<ATDPoolActor*> PoolActorArray;
		PoolActorArray.Add(TDPoolActor);
		ActorPool.Add(TDPoolActorClass, PoolActorArray);
	}

	TDPoolActor->OnAddedToPool();
}
