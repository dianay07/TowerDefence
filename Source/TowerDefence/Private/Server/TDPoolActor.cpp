#include "Server/TDPoolActor.h"


ATDPoolActor::ATDPoolActor()
{
 	PrimaryActorTick.bCanEverTick = true;

}

void ATDPoolActor::BeginPlay()
{
	Super::BeginPlay();
}


void ATDPoolActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATDPoolActor::OnAddedToPool()
{
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
}

void ATDPoolActor::OnRemovedFromPool()
{
	SetActorEnableCollision(true);
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	OnPoolSpawned();
}

