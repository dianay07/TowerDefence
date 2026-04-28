// Fill out your copyright notice in the Description page of Project Settings.

#include "TDProjectile.h"
#include "TDEnemyActor.h"
#include "TDFL_Utility.h"
#include "TDGameMode.h"

ATDProjectile::ATDProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	// 비복제 — 각 머신이 Multicast 수신 후 자기 로컬 인스턴스를 독립 시뮬레이션.
	// 데미지/HP 권위는 서버, 비주얼은 머신별 자체 진행.

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
    // 가드 제거 — 모든 머신이 자기 로컬 Projectile 을 독립적으로 이동시킨다.

    // [Image 1] Target 유효성 체크
    if (!IsValid(Target))
    {
        Target = nullptr;
        DespawnSelf();   // 서버=풀 반납, 클라=Destroy
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
    // 1. 데미지 — 서버 권위
    if (HasAuthority())
    {
        UTDFL_Utility::EnemyDamage(Target, Damage, BP_GE_DamageClass);
    }
    Target = nullptr;

    // 2. 정리 — 머신별로 (서버=풀, 클라=Destroy)
    DespawnSelf();
}

void ATDProjectile::DespawnSelf()
{
    if (HasAuthority())
    {
        if (ATDGameMode* GM = UTDFL_Utility::GetTDGameMode(this))
        {
            GM->PoolActor(this);
            return;
        }
        Destroy();   // 폴백 — 풀 미사용
    }
    else
    {
        Destroy();   // 클라 코스메틱 — 즉시 제거
    }
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
