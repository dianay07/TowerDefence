// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyActor.h"
#include "AbilitySystemComponent.h"
#include "TDEnemySet.h"

// Sets default values
AEnemyActor::AEnemyActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh"));
	RootComponent = StaticMeshComp;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("Ability System"));

	EnemySet = CreateDefaultSubobject<UTDEnemySet>(TEXT("EnemySet"));
}

// Called when the game starts or when spawned
void AEnemyActor::BeginPlay()
{
	Super::BeginPlay();
	
}

UAbilitySystemComponent* AEnemyActor::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AEnemyActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

}

// Called every frame
void AEnemyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

