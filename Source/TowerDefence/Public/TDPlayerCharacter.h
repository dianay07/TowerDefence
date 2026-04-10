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
class UUserWidget;

class ATDTowerPawn;

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
	// ── 컴포넌트 (카메라만) ──────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* CameraComp;

	// ── 플레이어 폰 (맵 위=캐릭터) ────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TSubclassOf<ATDPlayerPawn> PlayerPawnClass;

	UPROPERTY()
	ATDPlayerPawn* PlayerPawn;

	// ── UI ────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> HUDClass;

	UPROPERTY()
	UUserWidget* HUDWidget;

	UFUNCTION(BlueprintCallable, Category = "UI")
	FORCEINLINE UUserWidget* GetHUDWidget() const { return HUDWidget; }


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
	float EdgeScrollSpeed = 1000.f;

public:
	// ── 함수 ─────────────────────────────────────────────────
	void HandleCameraMove(const FInputActionValue& Value);
	void HandleClick();
	void TickEdgeScroll(float DeltaTime);

	// ── 포팅 진행중 함수 ─────────────────────────────────────────────────
	void SelectTower(ATDTowerPawn* Tower);
	void UnSelectTower();

	void OnCoinsChanged(int32 Change, int32 Coins);
	void OnHealthChanged(int32 HeartHealth, int32 MaxHeartHealth);

	// 적이 경로 끝 도달 시 외부에서 호출 → GameState 체력 감소
	UFUNCTION(BlueprintCallable, Category = "TD|Player")
	void NotifyBaseHealthDecreased();
};
