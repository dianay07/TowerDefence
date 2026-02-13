#include "TDPooledGameMode.h"

AActor* ATDPooledGameMode::GetPoolActorFromClass(TSubclassOf<AActor> ActorClass, FTransform Transform, AActor* NewOwner)
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

	if(PoolActor)
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

void ATDPooledGameMode::PoolActor(AActor* PoolActor)
{
	if(!PoolActor || !Cast<ITDPoolActorInterface>(PoolActor))
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
