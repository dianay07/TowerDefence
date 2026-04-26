#include "TDPlayerCharacter.h"
#include "TDPlayerPawn.h"
#include "TDTowerPawn.h"
#include "TDGameState.h"
#include "TDFL_Utility.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Blueprint/UserWidget.h"
#include "Net/UnrealNetwork.h"

// ── 생명주기 ──────────────────────────────────────────────────────────────────

ATDPlayerCharacter::ATDPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 루트: 카메라 앵커 전용 (물리 콜리전 없음)
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
	SetRootComponent(Root);

	// SpringArm: 탑다운 고정 각도
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->TargetArmLength = 600.f;
	SpringArmComp->SetRelativeRotation(FRotator(-50.f, 90.f, 0.f));
	SpringArmComp->bUsePawnControlRotation = false;
	SpringArmComp->bInheritPitch  = false;
	SpringArmComp->bInheritYaw    = false;
	SpringArmComp->bInheritRoll   = false;
	SpringArmComp->bDoCollisionTest = false;

	// 카메라: SpringArm 끝에 부착
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;
}

void ATDPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Enhanced Input 매핑 컨텍스트 등록 (로컬 플레이어만)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;

		if (UEnhancedInputLocalPlayerSubsystem* Sub =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Sub->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// 플레이어 폰 스폰 — 서버 권위, 복제로 클라에 전파
	if (HasAuthority() && PlayerPawnClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		PlayerPawn = GetWorld()->SpawnActor<ATDPlayerPawn>(
			PlayerPawnClass, GetActorLocation(), FRotator::ZeroRotator, Params);
	}

	// HUD 위젯 생성 — 로컬 플레이어만
	if (IsLocallyControlled() && HUDClass)
	{
		HUDWidget = CreateWidget(GetWorld(), HUDClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
		}
	}
}

void ATDPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TickEdgeScroll(DeltaTime);
}

void ATDPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATDPlayerCharacter::HandleCameraMove);
	}
}

void ATDPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATDPlayerCharacter, PlayerPawn);
}

// ── 카메라 & 엣지 스크롤 ──────────────────────────────────────────────────────

void ATDPlayerCharacter::GetCameraAxes(FVector& OutForward, FVector& OutRight) const
{
	// Roll 영향 없이 카메라 수평 방향 추출 (SpringArm Roll=90 등 케이스 대응)
	FVector CamForward = CameraComp->GetForwardVector();
	CamForward.Z = 0.f;
	OutForward = CamForward.GetSafeNormal();
	// UE 왼손계: Cross(Up, Forward) = Right
	OutRight = FVector::CrossProduct(FVector::UpVector, OutForward).GetSafeNormal();
}

void ATDPlayerCharacter::HandleCameraMove(const FInputActionValue& Value)
{
	// WASD 2D 입력 → 카메라 수평 패닝
	const FVector2D V = Value.Get<FVector2D>();
	FVector Forward, Right;
	GetCameraAxes(Forward, Right);
	SetActorLocation(GetActorLocation() + (Forward * V.Y + Right * V.X) * EdgeScrollSpeed * GetWorld()->GetDeltaSeconds());
}

void ATDPlayerCharacter::TickEdgeScroll(float DeltaTime)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	int32 VX, VY;
	PC->GetViewportSize(VX, VY);
	float MX, MY;
	if (!PC->GetMousePosition(MX, MY)) return;

	// 마우스가 뷰포트 가장자리에 닿으면 해당 방향으로 카메라 스크롤
	FVector2D Dir = FVector2D::ZeroVector;
	if      (MX < EdgeScrollThreshold)          Dir.X = -1.f;
	else if (MX > VX - EdgeScrollThreshold)     Dir.X =  1.f;
	if      (MY < EdgeScrollThreshold)          Dir.Y =  1.f;
	else if (MY > VY - EdgeScrollThreshold)     Dir.Y = -1.f;

	if (!Dir.IsZero())
	{
		FVector Forward, Right;
		GetCameraAxes(Forward, Right);
		SetActorLocation(GetActorLocation() + (Forward * Dir.Y + Right * Dir.X) * EdgeScrollSpeed * DeltaTime);
	}
}

// ── 플레이어 폰 / 기지 체력 ───────────────────────────────────────────────────

void ATDPlayerCharacter::NotifyBaseHealthDecreased()
{
	// 외부(BP/GameMode) 에서 호출 → GameState 권위 체력 감소
	if (ATDGameState* GS = UTDFL_Utility::GetTDGameState(this))
	{
		GS->DecreaseBaseHealth();
	}
}
