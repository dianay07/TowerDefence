// Fill out your copyright notice in the Description page of Project Settings.


#include "TDEnemyActor.h"
#include "AbilitySystemComponent.h"
#include "TDEnemySet.h"

// Sets default values
ATDEnemyActor::ATDEnemyActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneRootComp = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRootComp"));
	RootComponent = SceneRootComp;

	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh"));
	StaticMeshComp->SetupAttachment(RootComponent);


	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("Ability System"));

	EnemySet = CreateDefaultSubobject<UTDEnemySet>(TEXT("EnemySet"));
}

// Called when the game starts or when spawned
void ATDEnemyActor::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(EnemySet->GetHealthAttribute()).AddUObject(this, &ATDEnemyActor::OnHealthAttributeChanged);
}

UAbilitySystemComponent* ATDEnemyActor::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ATDEnemyActor::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	OnHealthChanged(Data.OldValue, Data.NewValue);
}

void ATDEnemyActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

}

// Called every frame
void ATDEnemyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

