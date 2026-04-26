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

// ── 생명주기 ──────────────────────────────────────────────────────────────────
public:
	ATDPlayerCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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

	// 카메라 기준 Forward/Right 벡터 계산 (Z 무시, SpringArm Roll 케이스 대응)
	void GetCameraAxes(FVector& OutForward, FVector& OutRight) const;

public:
	void HandleCameraMove(const FInputActionValue& Value);
	void TickEdgeScroll(float DeltaTime);

// ── 입력 ──────────────────────────────────────────────────────────────────────
private:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* MoveAction;  // WASD → 카메라 패닝 (클릭 이동은 ATDPlayerController 담당)

// ── 플레이어 폰 ───────────────────────────────────────────────────────────────
private:
	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TSubclassOf<ATDPlayerPawn> PlayerPawnClass;

	// 서버에서 스폰 후 복제 → 클라이언트 HandleClick 시 유효한 참조로 이동 명령 가능
	UPROPERTY(Replicated)
	ATDPlayerPawn* PlayerPawn;

public:
	ATDPlayerPawn* GetPlayerPawn() const { return PlayerPawn; }

	/** 기지 피격 시 외부(BP/EventManager) 에서 호출 → GameState->DecreaseBaseHealth(). */
	UFUNCTION(BlueprintCallable, Category = "TD|Player")
	void NotifyBaseHealthDecreased();

// ── UI ────────────────────────────────────────────────────────────────────────
private:
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> HUDClass;

	UPROPERTY()
	UUserWidget* HUDWidget;

public:
	UFUNCTION(BlueprintCallable, Category = "UI")
	FORCEINLINE UUserWidget* GetHUDWidget() const { return HUDWidget; }
};
