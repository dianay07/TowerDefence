#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "TDTowerPlacementSubsystem.generated.h"

class ATDTowerBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlacementStateChanged, bool, bIsPlacing);

/**
 * 타워 설치 프리뷰(유령 메시) 관리.
 * - 수명: LocalPlayer (플레이어 세션)
 * - 권한: 본인 클라만. 유령 메시는 복제하지 않음.
 * - 실제 설치는 PlayerController Server RPC로 위임.
 */
UCLASS()
class TOWERDEFENCE_API UTDTowerPlacementSubsystem
	: public ULocalPlayerSubsystem
	, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// ── ULocalPlayerSubsystem ────────────────────────────────────────────
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ── FTickableGameObject ──────────────────────────────────────────────
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
	virtual bool IsTickable() const override;
	virtual bool IsTickableInEditor() const override { return false; }
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTDTowerPlacementSubsystem, STATGROUP_Tickables); }
	virtual UWorld* GetTickableGameObjectWorld() const override;

	// ── Commands ─────────────────────────────────────────────────────────

	/** 설치 모드 시작 — 유령 메시 표시 */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerPlacement")
	void BeginPlacement(TSubclassOf<ATDTowerBase> TowerClass);

	/** 설치 확정 — Server RPC 전송 후 프리뷰 종료 */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerPlacement")
	void ConfirmPlacement();

	/** 설치 취소 — 유령 메시 제거 */
	UFUNCTION(BlueprintCallable, Category = "TD|TowerPlacement")
	void CancelPlacement();

	// ── Query ────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "TD|TowerPlacement")
	bool IsPlacing() const { return bIsPlacing; }

	UFUNCTION(BlueprintPure, Category = "TD|TowerPlacement")
	FVector GetPlacementLocation() const { return PlacementLocation; }

	UFUNCTION(BlueprintPure, Category = "TD|TowerPlacement")
	bool IsValidPlacement() const { return bIsValidPlacement; }

	// ── Events ───────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "TD|TowerPlacement")
	FOnPlacementStateChanged OnPlacementStateChanged;

private:
	/** 마우스 위치에 유령 메시 이동 + 설치 가능 여부 판정 */
	void UpdatePlacement();

	/** 유령 메시 액터 생성 */
	void SpawnGhostMesh();

	/** 유령 메시 액터 제거 */
	void DestroyGhostMesh();

	/** 마우스 아래 월드 좌표 구하기 */
	bool GetMouseWorldLocation(FVector& OutLocation) const;

	/** 유령 메시 머티리얼 색상 업데이트 (초록/빨강) */
	void UpdateGhostMaterial(bool bValid);

private:
	bool bIsPlacing = false;
	bool bIsValidPlacement = false;
	FVector PlacementLocation = FVector::ZeroVector;

	UPROPERTY()
	TSubclassOf<ATDTowerBase> PendingTowerClass;

	UPROPERTY()
	TObjectPtr<AActor> GhostMeshActor;
};
