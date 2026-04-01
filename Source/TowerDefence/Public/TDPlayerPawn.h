#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TDPlayerPawn.generated.h"

UCLASS()
class TOWERDEFENCE_API ATDPlayerPawn : public APawn
{
	GENERATED_BODY()

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* MeshComp;

public:
	ATDPlayerPawn();

	virtual void Tick(float DeltaTime) override;

	// ATDPlayerCharacter에서 호출 — 이동 목표 지점 설정
	// Phase 2: Server_SetMoveTarget RPC로 교체 예정 (HasAuthority 검증 추가)
	void SetMoveTarget(const FVector& WorldLocation);

protected:
	virtual void BeginPlay() override;

private:
	// Phase 2: UPROPERTY(ReplicatedUsing=OnRep_MoveTarget) 로 교체 예정
	FVector MoveTargetLocation;
	bool bIsMoving = false;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float MoveSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float AcceptanceRadius = 10.f;

	// 아래 방향 라인트레이스로 맵 표면 Z를 유지 (물리 없음)
	void SnapToGround();
};
