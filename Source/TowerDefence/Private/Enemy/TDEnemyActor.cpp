#include "Enemy/TDEnemyActor.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "Enemy/TDEnemySet.h"
#include "TDGameMode.h"
#include "TDGameState.h"
#include "Wave/TDPathActor.h"
#include "Net/UnrealNetwork.h"

ATDEnemyActor::ATDEnemyActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(false);			// Distance 기반 이동이므로 기본 이동 복제 비활성화

	// 컴포넌트 생성
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

	// 체력 속성 변경 감지 바인딩
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(EnemySet->GetHealthAttribute()).AddUObject(this, &ATDEnemyActor::OnHealthAttributeChanged);

	InitializeASC();
}

void ATDEnemyActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}

void ATDEnemyActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATDEnemyActor, CurrentPath);
	DOREPLIFETIME(ATDEnemyActor, Distance);
	DOREPLIFETIME(ATDEnemyActor, IsDead);
}

void ATDEnemyActor::OnRep_CurrentPath()
{
	if (CurrentPath)
		SetActorLocation(CurrentPath->GetLocation(Distance));
}

void ATDEnemyActor::OnRep_Distance()
{
	if (CurrentPath)
		SetActorLocation(CurrentPath->GetLocation(Distance));
}

void ATDEnemyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// ── GAS ──────────────────────────────────────────────────────────────────────

void ATDEnemyActor::InitializeASC()
{
	if (!DefaultEffect) return;

	// SetByCaller 태그로 초기 스탯 적용
	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultEffect, 1.f, AbilitySystemComponent->MakeEffectContext());
	if (!SpecHandle.IsValid()) return;

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, FGameplayTag::RequestGameplayTag(FName("Enemy.Health.SetByCaller")),    InitialHealth);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, FGameplayTag::RequestGameplayTag(FName("Enemy.MoveSpeed.SetByCaller")), InitialMoveSpeed);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, FGameplayTag::RequestGameplayTag(FName("Enemy.Damage.SetByCaller")),    InitialDamage);

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

	// 체력이 0 이하가 되면 사망 처리
	if (Data.NewValue <= 0.f && !IsDead && !IsActorBeingDestroyed())
	{
		IsDead = true;
		OnEnemyDied();
	}
}

// ── 사망 처리 ─────────────────────────────────────────────────────────────────

void ATDEnemyActor::OnEnemyDied()
{
	if (IsActorBeingDestroyed()) return;

	// 보상 코인 지급
	if (ATDGameState* GS = GetWorld()->GetGameState<ATDGameState>())
		GS->CoinChange(RewardCoin);

	// 사망 이벤트 브로드캐스트 (WaveManager 목록 제거)
	OnDied.Broadcast(this);

	// 이동/충돌 즉시 중단 후 애니메이션 재생, DeathDelay 후 파괴
	SetActorEnableCollision(false);
	PlayDeathAnimation();

	GetWorldTimerManager().SetTimer(
		DeathTimerHandle,
		this, &ATDEnemyActor::DestroyAfterDelay,
		DeathDelay, false);
}

void ATDEnemyActor::DestroyAfterDelay()
{
	if (!IsActorBeingDestroyed())
		Destroy();
}

// ── 경로 이동 ─────────────────────────────────────────────────────────────────

void ATDEnemyActor::InitializePath(ATDPath* Path)
{
	// 경로 초기화 및 시작 위치 배치
	CurrentPath = Path;
	Distance = 0.f;

	if (CurrentPath)
		SetActorLocation(CurrentPath->GetLocation(0.f));
}

float ATDEnemyActor::Advance(float DeltaTime)
{
	if (!HasAuthority()) return Distance;
	if (!CurrentPath || IsDead) return Distance;

	// 이동 속도에 따라 경로 진행
	Distance += EnemySet->GetMoveSpeed() * DeltaTime;

	// 경로 끝 도달 시 기지 체력 감소
	if (Distance >= CurrentPath->GetLength())
	{
		if (ATDGameState* GS = GetWorld()->GetGameState<ATDGameState>())
			GS->DecreaseBaseHealth();

		if (ATDGameMode* GM = Cast<ATDGameMode>(GetWorld()->GetAuthGameMode()))
			GM->CheckIfLoss();

		return Distance;
	}

	SetActorLocation(CurrentPath->GetLocation(Distance));
	return Distance;
}
