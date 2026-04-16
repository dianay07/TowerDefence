// Fill out your copyright notice in the Description page of Project Settings.

#include "TDProjectile.h"
#include "TDEnemyActor.h"
#include "TDFL_Utility.h"
#include "TDGameMode.h"

ATDProjectile::ATDProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(DefaultSceneRoot);
}

void ATDProjectile::OnAddedToPool()
{
	Super::OnAddedToPool();
}

void ATDProjectile::OnRemovedFromPool()
{
	Super::OnRemovedFromPool();  // OnPoolSpawned() 호출 포함

	// Event OnPoolSpawned → SET LastDistance = 99999
	LastDistance = 99999.f;
}

void ATDProjectile::BeginPlay()
{
	Super::BeginPlay();
}

void ATDProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    MoveTowardsTarget(DeltaTime);
}

void ATDProjectile::MoveTowardsTarget(float Delta)
{
    // [Image 1] Target 유효성 체크
    if (!IsValid(Target))
    {
        Target = nullptr;
        // 타겟이 없는 경우 미사일을 PoolActor 에 다시 등록
        ATDGameMode* GM = UTDFL_Utility::GetTDGameMode(this);
        if (!IsValid(GM))
        {
            return;
        }
        GM->PoolActor(this);

        return;
    }

    // [Image 2] 방향 및 거리 계산
    FVector SelfLocation   = GetActorLocation();
    FVector TargetLocation = Target->GetActorLocation();
    FVector Direction      = TargetLocation - SelfLocation;
    float Distance         = Direction.Length();
    Direction.Normalize();

    // [Image 2] Target이 숨겨진(풀 반납) 경우 OR 목표를 지나친 경우 → OnHitTarget
    //bool bTargetHidden = Target->IsHidden();
    bool bOvershot     = (Distance < 20.f) || (Distance > LastDistance);

    if (bOvershot)
    {
        OnHitTarget();
        LastDistance = 99999.f;
        return;
    }

    // [Image 2] 새 위치 계산: CurrentLocation + Direction * Speed * DeltaTime
    FVector NewLocation = SelfLocation + Direction * Speed * Delta;

    // [Image 3] Set Actor Location
    SetActorLocation(NewLocation);

    // [Image 3] Target 방향으로 회전: Rotation From X Vector
    FRotator NewRotation = FRotationMatrix::MakeFromX(Direction).Rotator();
    SetActorRotation(NewRotation);

    // [Image 3] SET Last Distance = Distance
    LastDistance = Distance;
}
void ATDProjectile::OnHitTarget()
{
    // 1. Damage Enemy
    UTDFL_Utility::EnemyDamage(Target, Damage, BP_GE_DamageClass);
    Target = nullptr;

    // 2. Get Game Mode → Pool Actor(Self)
    ATDGameMode* GM = UTDFL_Utility::GetTDGameMode(this);
    if (!IsValid(GM))
    {
        return;
    }
    GM->PoolActor(this);
}
void ATDProjectile::SetProjectileData(ATDEnemyActor* InTarget, float InDamage, float InRadius)
{
    // Cast To BP_EnemyBase → Cast Failed 시 리턴
    ATDEnemyActor* CastedTarget = Cast<ATDEnemyActor>(InTarget);
    if (!IsValid(CastedTarget))
    {
        return;
    }

    // SET Target, Damage, Radius
    Target = CastedTarget;
    Damage = InDamage;
    Radius = InRadius;
}
