#include "TDEnemyActor.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "TDEnemySet.h"
#include "TDGameMode.h"

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

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
			FString::Printf(TEXT("[%s] Health=%.1f | MoveSpeed=%.1f | Damage=%.1f"),
				*GetName(), InitialHealth, InitialMoveSpeed, InitialDamage));
	}
}

UAbilitySystemComponent* ATDEnemyActor::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ATDEnemyActor::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	OnHealthChanged(Data.OldValue, Data.NewValue);

	if (Data.NewValue <= 0.f && !IsDead)
	{
		IsDead = true;
		OnEnemyDied();
	}
}

void ATDEnemyActor::OnEnemyDied()
{
	OnDied.Broadcast();
	PlayDeathAnimation();

	if (ATDGameMode* GM = Cast<ATDGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->EventManager->BroadcastEnemyDied(this);
	}
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
