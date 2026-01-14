#include "TDTowerPawn.h"
#include "Components/StaticMeshComponent.h"
#include "AbilitySystemComponent.h"

ATDTowerPawn::ATDTowerPawn()
{
 	PrimaryActorTick.bCanEverTick = true;

	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh"));
	RootComponent = StaticMeshComp;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("Ability System"));
}

void ATDTowerPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

UAbilitySystemComponent* ATDTowerPawn::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ATDTowerPawn::InitializeASC()
{
	if (HasAuthority() && DefaultAbility)
	{
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DefaultAbility, 1, -1, this));
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultEffect, 1, EffectContext);
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}

}

void ATDTowerPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	InitializeASC();
}

void ATDTowerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
