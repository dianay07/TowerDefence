#include "TDTowerPawn.h"
#include "AbilitySystemComponent.h"
#include "TDTowerSet.h"

ATDTowerPawn::ATDTowerPawn()
{
 	PrimaryActorTick.bCanEverTick = true;

	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh"));
	RootComponent = StaticMeshComp;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("Ability System"));

	TowerSet = CreateDefaultSubobject<UTDTowerSet>(TEXT("TowerSet")); 
}

void ATDTowerPawn::BeginPlay()
{
	Super::BeginPlay();
	
	InitializeASC();
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

	//Note(Jiho): 값이 설정되어 있지 않은 상태에서 호출되어 문제 되는 코드라 주석 처리.
	//FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultEffect, 1, EffectContext);
	//if (SpecHandle.IsValid())
	//{
	//	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	//}
}

void ATDTowerPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}

void ATDTowerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
