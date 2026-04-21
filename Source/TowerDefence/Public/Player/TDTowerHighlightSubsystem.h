#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "TDTowerHighlightSubsystem.generated.h"

class ATDTowerBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHoveredTowerChanged, ATDTowerBase*, NewTower, ATDTowerBase*, OldTower);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSelectedTowerChanged, ATDTowerBase*, NewTower, ATDTowerBase*, OldTower);

/**
 * 마우스 호버 하이라이트 및 타워 선택 상태 관리.
 * - 수명: LocalPlayer (플레이어 세션)
 * - 권한: 본인 클라만. 다른 플레이어에게 영향 없음.
 * - 상태 변경 요청이 필요하면 PlayerController 의 Server RPC 로 위임.
 */
UCLASS()
class TOWERDEFENCE_API UTDTowerHighlightSubsystem
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
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTDTowerHighlightSubsystem, STATGROUP_Tickables); }
	virtual UWorld* GetTickableGameObjectWorld() const override;

	// ── Query ────────────────────────────────────────────────────────────
	UFUNCTION(BlueprintPure, Category = "TD|TowerHighlight")
	ATDTowerBase* GetHoveredTower() const { return HoveredTower.Get(); }

	UFUNCTION(BlueprintPure, Category = "TD|TowerHighlight")
	ATDTowerBase* GetSelectedTower() const { return SelectedTower.Get(); }

	// ── Commands (로컬 UI 상태만 변경) ───────────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "TD|TowerHighlight")
	void SelectTower(ATDTowerBase* Tower);

	UFUNCTION(BlueprintCallable, Category = "TD|TowerHighlight")
	void UnSelectTower();

	// ── Events ───────────────────────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category = "TD|TowerHighlight")
	FOnHoveredTowerChanged  OnHoveredTowerChanged;

	UPROPERTY(BlueprintAssignable, Category = "TD|TowerHighlight")
	FOnSelectedTowerChanged OnSelectedTowerChanged;

private:
	void UpdateHover();
	void SetHoveredTower(ATDTowerBase* NewTower);
	void ApplyHighlight(ATDTowerBase* Tower, bool bOn);

private:
	// GC 를 방해하지 않도록 Weak 유지. 삭제된 타워는 자동으로 null.
	TWeakObjectPtr<ATDTowerBase> HoveredTower;
	TWeakObjectPtr<ATDTowerBase> SelectedTower;
};
