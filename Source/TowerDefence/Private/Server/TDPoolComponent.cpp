#include "Server/TDPoolComponent.h"
#include "TDPoolActorInterface.h"

UTDPoolComponent::UTDPoolComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

AActor* UTDPoolComponent::GetPoolActorFromClass(TSubclassOf<AActor> ActorClass, FTransform Transform, AActor* NewOwner)
{
	if (!ActorClass || !ActorClass->ImplementsInterface(UTDPoolActorInterface::StaticClass()))
	{
		return nullptr;
	}

	AActor* PoolActor = nullptr;

	// 풀에 재사용 가능한 Actor가 있으면 꺼내 사용
	if (ActorPool.Contains(ActorClass))
	{
		TArray<AActor*>* PoolActorArray = ActorPool.Find(ActorClass);
		if (PoolActorArray && !PoolActorArray->IsEmpty())
		{
			PoolActor = PoolActorArray->Pop();
		}
	}

	if (!PoolActor)
	{
		PoolActor = GetWorld()->SpawnActor<AActor>(ActorClass);
	}

	if (PoolActor)
	{
		PoolActor->SetActorTransform(Transform);
		PoolActor->SetOwner(NewOwner);

		ITDPoolActorInterface* PoolInterface = Cast<ITDPoolActorInterface>(PoolActor);
		if (PoolInterface)
		{
			PoolInterface->OnRemovedFromPool();
		}
	}

	return PoolActor;
}

void UTDPoolComponent::ReturnToPool(AActor* PoolActor)
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
		TArray<AActor*> NewPool;
		NewPool.Add(PoolActor);
		ActorPool.Add(ActorClass, NewPool);
	}

	Cast<ITDPoolActorInterface>(PoolActor)->OnAddedToPool();
}
