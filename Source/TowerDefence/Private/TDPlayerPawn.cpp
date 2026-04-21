#include "TDPlayerPawn.h"
#include "Net/UnrealNetwork.h"

ATDPlayerPawn::ATDPlayerPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true); // 서버 권위 이동 → 클라에 자동 위치 복제

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	SetRootComponent(MeshComp);

	// 물리/중력 없음 — SnapToGround로 맵 표면 유지
	MeshComp->SetSimulatePhysics(false);
	MeshComp->SetEnableGravity(false);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATDPlayerPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATDPlayerPawn, MoveTargetLocation);
}

void ATDPlayerPawn::BeginPlay()
{
	Super::BeginPlay();
	SnapToGround();
}

void ATDPlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 서버에서만 이동 처리 — 위치는 SetReplicateMovement로 자동 전파
	if (!HasAuthority()) return;
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
	if (HasAuthority())
	{
		// 서버(Listen Server 호스트)는 직접 설정
		MoveTargetLocation = WorldLocation;
		bIsMoving = true;
	}
	else
	{
		// 순수 클라이언트는 Server RPC 요청
		Server_SetMoveTarget(WorldLocation);
	}
}

void ATDPlayerPawn::Server_SetMoveTarget_Implementation(FVector WorldLocation)
{
	MoveTargetLocation = WorldLocation;
	bIsMoving = true;
}

void ATDPlayerPawn::OnRep_MoveTarget()
{
	// 클라이언트: 서버에서 목표 위치가 복제되면 이동 시작
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
