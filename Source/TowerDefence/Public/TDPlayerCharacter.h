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

// ── 카메라 & 엣지 스크롤 ──────────────────────────────────────────────────────
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* CameraComp;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|EdgeScroll")
	float EdgeScrollThreshold = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|EdgeScroll")
	float EdgeScrollSpeed = 1000.f;

private:
	// 카메라 기준 Forward/Right 벡터 계산 (Z 무시)
	void GetCameraAxes(FVector& OutForward, FVector& OutRight) const;

public:
	void HandleCameraMove(const FInputActionValue& Value);
	void TickEdgeScroll(float DeltaTime);

// ── 입력 ──────────────────────────────────────────────────────────────────────
private:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* MoveAction;		// WASD → 카메라 패닝

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* ClickAction;		// 좌클릭 → 플레이어 폰 이동

public:
	void HandleClick();

// ── 플레이어 폰 ───────────────────────────────────────────────────────────────
private:
	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TSubclassOf<ATDPlayerPawn> PlayerPawnClass;

	UPROPERTY()
	ATDPlayerPawn* PlayerPawn;

public:
	void SelectTower(ATDTowerPawn* Tower);
	void UnSelectTower();

	UFUNCTION(BlueprintCallable, Category = "TD|Player")
	void NotifyBaseHealthDecreased();

	void OnCoinsChanged(int32 Change, int32 Coins);
	void OnHealthChanged(int32 HeartHealth, int32 MaxHeartHealth);

// ── UI ────────────────────────────────────────────────────────────────────────
private:
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> HUDClass;

	UPROPERTY()
	UUserWidget* HUDWidget;

	UFUNCTION(BlueprintCallable, Category = "UI")
	FORCEINLINE UUserWidget* GetHUDWidget() const { return HUDWidget; }
};
