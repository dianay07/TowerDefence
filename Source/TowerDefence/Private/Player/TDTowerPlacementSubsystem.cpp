#include "Player/TDTowerPlacementSubsystem.h"
#include "TDTowerBase.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

// ── ULocalPlayerSubsystem ────────────────────────────────────────────────────

void UTDTowerPlacementSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UTDTowerPlacementSubsystem::Deinitialize()
{
	CancelPlacement();
	Super::Deinitialize();
}

// ── FTickableGameObject ──────────────────────────────────────────────────────

bool UTDTowerPlacementSubsystem::IsTickable() const
{
	const ULocalPlayer* LP = GetLocalPlayer<ULocalPlayer>();
	return LP && LP->GetWorld() && bIsPlacing;
}

UWorld* UTDTowerPlacementSubsystem::GetTickableGameObjectWorld() const
{
	if (const ULocalPlayer* LP = GetLocalPlayer<ULocalPlayer>())
	{
		return LP->GetWorld();
	}
	return nullptr;
}

void UTDTowerPlacementSubsystem::Tick(float DeltaTime)
{
	if (bIsPlacing)
	{
		UpdatePlacement();
	}
}

// ── Commands ─────────────────────────────────────────────────────────────────

void UTDTowerPlacementSubsystem::BeginPlacement(TSubclassOf<ATDTowerBase> TowerClass)
{
	if (!TowerClass || bIsPlacing)
	{
		return;
	}

	PendingTowerClass = TowerClass;
	bIsPlacing = true;
	bIsValidPlacement = false;

	SpawnGhostMesh();

	OnPlacementStateChanged.Broadcast(true);
}

void UTDTowerPlacementSubsystem::ConfirmPlacement()
{
	if (!bIsPlacing || !bIsValidPlacement)
	{
		return;
	}

	// TODO: PlayerController Server RPC 호출
	// ATDPlayerController* PC = ...;
	// PC->ServerPlaceTower(PendingTowerClass, PlacementLocation);

	DestroyGhostMesh();
	bIsPlacing = false;
	PendingTowerClass = nullptr;

	OnPlacementStateChanged.Broadcast(false);
}

void UTDTowerPlacementSubsystem::CancelPlacement()
{
	if (!bIsPlacing)
	{
		return;
	}

	DestroyGhostMesh();
	bIsPlacing = false;
	bIsValidPlacement = false;
	PendingTowerClass = nullptr;

	OnPlacementStateChanged.Broadcast(false);
}

// ── Internal ─────────────────────────────────────────────────────────────────

void UTDTowerPlacementSubsystem::UpdatePlacement()
{
	FVector MouseWorldLoc;
	if (!GetMouseWorldLocation(MouseWorldLoc))
	{
		bIsValidPlacement = false;
		UpdateGhostMaterial(false);
		return;
	}

	PlacementLocation = MouseWorldLoc;

	// 유령 메시 위치 업데이트
	if (IsValid(GhostMeshActor))
	{
		GhostMeshActor->SetActorLocation(PlacementLocation);
	}

	// 설치 가능 여부 판정 — 현재 위치에 타워가 없으면 설치 가능
	// TODO: 슬롯 기반이라면 가장 가까운 빈 슬롯 검색으로 변경
	bIsValidPlacement = true;

	UpdateGhostMaterial(bIsValidPlacement);
}

void UTDTowerPlacementSubsystem::SpawnGhostMesh()
{
	UWorld* World = GetTickableGameObjectWorld();
	if (!World || !PendingTowerClass)
	{
		return;
	}

	// CDO에서 메시 참조를 가져와 유령 메시 액터 생성
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	GhostMeshActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!IsValid(GhostMeshActor))
	{
		return;
	}

	// 루트 컴포넌트
	USceneComponent* Root = NewObject<USceneComponent>(GhostMeshActor, TEXT("GhostRoot"));
	GhostMeshActor->SetRootComponent(Root);
	Root->RegisterComponent();

	// CDO의 StaticMesh를 복사하여 유령 메시로 사용
	const ATDTowerBase* CDO = PendingTowerClass.GetDefaultObject();
	if (CDO)
	{
		// TDTowerPawn의 StaticMeshComp에서 메시 가져오기
		if (const UStaticMeshComponent* SrcMesh = CDO->FindComponentByClass<UStaticMeshComponent>())
		{
			UStaticMeshComponent* GhostMesh = NewObject<UStaticMeshComponent>(GhostMeshActor, TEXT("GhostMesh"));
			GhostMesh->SetupAttachment(Root);
			GhostMesh->SetStaticMesh(const_cast<UStaticMesh*>(SrcMesh->GetStaticMesh()));
			GhostMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			GhostMesh->RegisterComponent();
		}
	}

	// 충돌 없음 — 프리뷰 전용
	GhostMeshActor->SetActorEnableCollision(false);
}

void UTDTowerPlacementSubsystem::DestroyGhostMesh()
{
	if (IsValid(GhostMeshActor))
	{
		GhostMeshActor->Destroy();
		GhostMeshActor = nullptr;
	}
}

bool UTDTowerPlacementSubsystem::GetMouseWorldLocation(FVector& OutLocation) const
{
	const ULocalPlayer* LP = GetLocalPlayer<ULocalPlayer>();
	if (!LP)
	{
		return false;
	}

	APlayerController* PC = LP->GetPlayerController(LP->GetWorld());
	if (!PC)
	{
		return false;
	}

	FHitResult Hit;
	if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		OutLocation = Hit.Location;
		return true;
	}

	return false;
}

void UTDTowerPlacementSubsystem::UpdateGhostMaterial(bool bValid)
{
	if (!IsValid(GhostMeshActor))
	{
		return;
	}

	UStaticMeshComponent* MeshComp = GhostMeshActor->FindComponentByClass<UStaticMeshComponent>();
	if (!MeshComp)
	{
		return;
	}

	// 설치 가능: 반투명 초록 / 불가: 반투명 빨강
	FLinearColor Color = bValid
		? FLinearColor(0.f, 1.f, 0.f, 0.3f)
		: FLinearColor(1.f, 0.f, 0.f, 0.3f);

	UMaterialInstanceDynamic* DynMat = MeshComp->CreateAndSetMaterialInstanceDynamic(0);
	if (DynMat)
	{
		DynMat->SetVectorParameterValue(TEXT("Color"), Color);
		DynMat->SetScalarParameterValue(TEXT("Opacity"), Color.A);
	}
}
