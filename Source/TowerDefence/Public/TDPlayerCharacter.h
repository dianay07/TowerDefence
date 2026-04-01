#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "TDPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class ATDPlayerPawn;

UCLASS()
class TOWERDEFENCE_API ATDPlayerCharacter : public APawn
{
	GENERATED_BODY()

public:
	ATDPlayerCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	// ── 컴포넌트 (카메라만, 메시 없음) ──────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* CameraComp;

	// ── 플레이어 폰 (맵 위 시각적 캐릭터) ────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TSubclassOf<ATDPlayerPawn> PlayerPawnClass;

	UPROPERTY()
	ATDPlayerPawn* PlayerPawn;

	// ── 입력 ─────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* MoveAction;		// WASD → 카메라 패닝

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* ClickAction;		// 좌클릭 → 플레이어 폰 이동

	// ── Edge Scroll ──────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "Camera|EdgeScroll")
	float EdgeScrollThreshold = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|EdgeScroll")
	float EdgeScrollSpeed = 1500.f;

	// ── 함수 ─────────────────────────────────────────────────
	void HandleCameraMove(const FInputActionValue& Value);
	void HandleClick();
	void TickEdgeScroll(float DeltaTime);
};
