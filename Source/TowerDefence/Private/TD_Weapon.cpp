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
	// GET Target → Is Valid
	if (!IsValid(Target) || !IsValid(Tower) || !ProjectileClass)
	{
		return;
	}

	// Get Fire Point → Location, Rotation
	FVector  FireLocation;
	FRotator FireRotation;
	GetFirePoint(FireLocation, FireRotation);

	FTransform SpawnTransform(FireRotation, FireLocation, FVector::OneVector);

	// Get Pool Actor from Class via UTDFL_Utility
	ATDGameMode* GM = UTDFL_Utility::GetTDGameMode(this);
	if (!IsValid(GM))
	{
		return;
	}

	AActor* PooledActor = GM->GetPoolActorFromClass(ProjectileClass, SpawnTransform, this);
	if (!IsValid(PooledActor))
	{
		return;
	}

	// SET Projectile
	Projectile = PooledActor;

	// BP 함수 SetProjectileData(Target, Damage, Radius) 호출
	/*if (UFunction* Func = PooledActor->FindFunction(TEXT("SetProjectileData")))
	{
		struct FSetProjectileDataParams
		{
			AActor* Target;
			float Damage;
			float Radius;
		};

		FSetProjectileDataParams Params;
		Params.Target = Target;
		Params.Damage = Tower->GetDamage();
		Params.Radius = Tower->GetRadius();

		PooledActor->ProcessEvent(Func, &Params);
	}*/

	// 프로퍼티 직접 설정 (BP 변수명 기준)
	if (FObjectProperty* TargetProp = FindFProperty<FObjectProperty>(PooledActor->GetClass(), TEXT("Target")))
	{
		TargetProp->SetObjectPropertyValue_InContainer(PooledActor, Target);
	}
	if (FFloatProperty* DamageProp = FindFProperty<FFloatProperty>(PooledActor->GetClass(), TEXT("Damage")))
	{
		DamageProp->SetPropertyValue_InContainer(PooledActor, Tower->GetDamage());
	}
	if (FFloatProperty* RadiusProp = FindFProperty<FFloatProperty>(PooledActor->GetClass(), TEXT("Radius")))
	{
		RadiusProp->SetPropertyValue_InContainer(PooledActor, Tower->GetRadius());
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
