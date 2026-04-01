#include "TDPlayerPawn.h"

ATDPlayerPawn::ATDPlayerPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	SetRootComponent(MeshComp);

	// 물리/중력 없음 — SnapToGround로 맵 표면 유지
	MeshComp->SetSimulatePhysics(false);
	MeshComp->SetEnableGravity(false);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Phase 2: SetReplicates(true), bAlwaysRelevant = true 추가 예정
}

void ATDPlayerPawn::BeginPlay()
{
	Super::BeginPlay();
	SnapToGround();
}

void ATDPlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsMoving) return;

	FVector Current = GetActorLocation();
	FVector Dir = MoveTargetLocation - Current;
	Dir.Z = 0.f;

	if (Dir.Size() < AcceptanceRadius)
	{
		bIsMoving = false;
		SnapToGround();
		return;
	}

	SetActorLocation(Current + Dir.GetSafeNormal() * MoveSpeed * DeltaTime);
	SnapToGround();
}

void ATDPlayerPawn::SetMoveTarget(const FVector& WorldLocation)
{
	// Phase 2 교체 구조:
	//   if (!HasAuthority()) { Server_SetMoveTarget(WorldLocation); return; }
	//   (서버에서 위치 검증 후 MoveTargetLocation 설정 → Replicated로 전파)
	MoveTargetLocation = WorldLocation;
	bIsMoving = true;
}

void ATDPlayerPawn::SnapToGround()
{
	FVector Start = GetActorLocation() + FVector(0.f, 0.f, 200.f);
	FVector End   = GetActorLocation() - FVector(0.f, 0.f, 500.f);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
	{
		FVector Loc = GetActorLocation();
		Loc.Z = Hit.Location.Z;
		SetActorLocation(Loc);
	}
}
