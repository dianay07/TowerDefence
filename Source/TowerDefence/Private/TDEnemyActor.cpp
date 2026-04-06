#include "TDEnemyActor.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "TDEnemySet.h"

ATDEnemyActor::ATDEnemyActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRootComp = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRootComp"));
	RootComponent = SceneRootComp;

	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh"));
	StaticMeshComp->SetupAttachment(RootComponent);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("Ability System"));
	EnemySet = CreateDefaultSubobject<UTDEnemySet>(TEXT("EnemySet"));
}

void ATDEnemyActor::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(EnemySet->GetHealthAttribute()).AddUObject(this, &ATDEnemyActor::OnHealthAttributeChanged);

	InitializeASC();
}

void ATDEnemyActor::InitializeASC()
{
	if (!DefaultEffect) return;
	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultEffect, 1.f, AbilitySystemComponent->MakeEffectContext());

	if (!SpecHandle.IsValid()) return;

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle, FGameplayTag::RequestGameplayTag(FName("Enemy.Health.SetByCaller")),    InitialHealth);

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle, FGameplayTag::RequestGameplayTag(FName("Enemy.MoveSpeed.SetByCaller")), InitialMoveSpeed);

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		SpecHandle, FGameplayTag::RequestGameplayTag(FName("Enemy.Damage.SetByCaller")),    InitialDamage);

	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
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

void ATDEnemyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
