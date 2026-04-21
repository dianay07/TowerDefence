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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 로컬 클라이언트에서 호출 → Server RPC를 통해 서버에서 목표 위치 설정
	void SetMoveTarget(const FVector& WorldLocation);

protected:
	virtual void BeginPlay() override;

private:
	// 서버에서 설정 → 클라이언트로 복제 → OnRep_MoveTarget에서 이동 시작
	UPROPERTY(ReplicatedUsing=OnRep_MoveTarget)
	FVector MoveTargetLocation;

	bool bIsMoving = false;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float MoveSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float AcceptanceRadius = 10.f;

	UFUNCTION()
	void OnRep_MoveTarget();

	// 서버 RPC — 클라이언트가 서버에 이동 목표 요청
	UFUNCTION(Server, Reliable)
	void Server_SetMoveTarget(FVector WorldLocation);

	// 아래 방향 라인트레이스로 맵 표면 Z를 유지 (물리 없음)
	void SnapToGround();
};
