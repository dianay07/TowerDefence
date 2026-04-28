// Fill out your copyright notice in the Description page of Project Settings.

#include "TD_Weapon.h"
#include "TDEnemyActor.h"
#include "TDTowerBase.h"
#include "TDFL_Utility.h"
#include "TDWaveManagerComponent.h"
#include "TDGameMode.h"
#include "Kismet/KismetMathLibrary.h"

ATD_Weapon::ATD_Weapon()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;             // 클라이언트에 Actor 복제
	SetReplicateMovement(true);     // 서버 회전(조준)을 클라이언트에 동기화

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	Bottom = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bottom"));
	Bottom->SetupAttachment(DefaultSceneRoot);

	Top = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Top"));
	Top->SetupAttachment(Bottom);
}

void ATD_Weapon::BeginPlay()
{
	Super::BeginPlay();
}

void ATD_Weapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!HasAuthority()) return;   // 타겟 검색/조준은 서버 권한

	FindEnemy();
	FaceEnemy();
}

void ATD_Weapon::FindEnemy()
{
	// Get Wave Manager C++ → Is Valid Branch
	UTDWaveManagerComponent* WaveManager = UTDFL_Utility::GetWaveManager(this);
	if (!IsValid(WaveManager))
	{
		return;
	}

	// Get Furthest Enemy
	// Location = Self Actor Location
	// Radius   = Tower->GetRange() + 100 (BP 주석: 거리체크 xy/xyz 정리필요)
	float Range = IsValid(Tower) ? Tower->GetRange() + 100.f : 0.f;
	ATDEnemyActor* Found = WaveManager->GetFurthestEnemy(GetActorLocation(), Range);

	// Return Value → Is Valid Branch → SET Target
	if (IsValid(Found))
	{
		Target = Found;
	}
}

void ATD_Weapon::FaceEnemy()
{
	if (!IsValid(Target))
	{
		Target = nullptr;
		return;
	}

	// Z를 0으로 고정 → Yaw(수평) 회전만 적용
	FVector Start  = GetActorLocation();
	FVector TargetLoc = Target->GetActorLocation();
	Start.Z     = 0.f;
	TargetLoc.Z = 0.f;

	// Find Look at Rotation → Set Actor Rotation (Self)
	FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(Start, TargetLoc);
	SetActorRotation(LookAtRot);
}

void ATD_Weapon::FireAtEnemy()
{
	if (!HasAuthority()) return;   // 발사 결정은 서버 권한

	if (!IsValid(Target) || !IsValid(Tower) || !ProjectileClass)
	{
		Target = nullptr;
		return;
	}

	FVector  FireLocation;
	FRotator FireRotation;
	GetFirePoint(FireLocation, FireRotation);

	// 모든 머신에 전파 — 각자 로컬 Projectile 스폰 (비복제 코스메틱)
	MulticastFireProjectile(Target, Tower->GetDamage(), Tower->GetRadius(),
	                        FireLocation, FireRotation);
	Target = nullptr;
}

void ATD_Weapon::MulticastFireProjectile_Implementation(
	ATDEnemyActor* InTarget, float InDamage, float InRadius,
	FVector_NetQuantize SpawnLocation, FRotator SpawnRotation)
{
	if (!ProjectileClass || !IsValid(InTarget))
	{
		return;
	}

	const FTransform SpawnTransform(SpawnRotation, FVector(SpawnLocation), FVector::OneVector);
	ATDProjectile* P = nullptr;

	if (HasAuthority())
	{
		// 서버: 기존 풀 사용
		ATDGameMode* GM = UTDFL_Utility::GetTDGameMode(this);
		if (!IsValid(GM)) return;

		P = Cast<ATDProjectile>(GM->GetPoolActorFromClass(ProjectileClass, SpawnTransform, this));
	}
	else
	{
		// 클라: 풀 없음 → 단순 SpawnActor (코스메틱)
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.Owner = this;
		P = GetWorld()->SpawnActor<ATDProjectile>(ProjectileClass, SpawnTransform, Params);
	}

	if (!IsValid(P)) return;

	P->SetProjectileData(InTarget, InDamage, InRadius);

	// 서버에서만 Weapon 의 Projectile 캐시 갱신 (BP 가 참조할 가능성 대비)
	if (HasAuthority())
	{
		Projectile = P;
	}
}

void ATD_Weapon::GetFirePoint(FVector& OutLocation, FRotator& OutRotation)
{
	// [Then 0] FirePointIndex >= FirePoints.Length 이면 0으로 리셋
	if (FirePointIndex >= FirePoints.Num())
	{
		FirePointIndex = 0;
	}

	// [Then 1] FirePoints[FirePointIndex] GET → FirePointIndex++
	USceneComponent* FirePoint = FirePoints.IsValidIndex(FirePointIndex) ? FirePoints[FirePointIndex] : nullptr;
	FirePointIndex++;

	// Is Valid → World Location / Rotation
	// Not Valid → Self Actor Location / Rotation
	if (IsValid(FirePoint))
	{
		OutLocation = FirePoint->GetComponentLocation();
		OutRotation = FirePoint->GetComponentRotation();
	}
	else
	{
		OutLocation = GetActorLocation();
		OutRotation = GetActorRotation();
	}
}
