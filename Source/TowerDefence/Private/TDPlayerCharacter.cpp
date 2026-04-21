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

ATDPlayerCharacter::ATDPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 루트: 카메라 앵커 전용
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
	SetRootComponent(Root);

	// SpringArm 설정
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->TargetArmLength = 600.f;
	SpringArmComp->SetRelativeRotation(FRotator(-50.f, 90.f, 0.f));
	SpringArmComp->bUsePawnControlRotation = false;
	SpringArmComp->bInheritPitch  = false;
	SpringArmComp->bInheritYaw    = false;
	SpringArmComp->bInheritRoll   = false;
	SpringArmComp->bDoCollisionTest = false;

	// 카메라 설정
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;
}

void ATDPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATDPlayerCharacter, PlayerPawn);
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

	// 플레이어 폰 스폰 — 서버에서만 생성 후 복제
	if (HasAuthority() && PlayerPawnClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		PlayerPawn = GetWorld()->SpawnActor<ATDPlayerPawn>(PlayerPawnClass, GetActorLocation(), FRotator::ZeroRotator, Params);
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

	// Enhanced Input 액션 바인딩
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(MoveAction,  ETriggerEvent::Triggered, this, &ATDPlayerCharacter::HandleCameraMove);
		EIC->BindAction(ClickAction, ETriggerEvent::Started,   this, &ATDPlayerCharacter::HandleClick);
	}
}

// ── 카메라 이동 ───────────────────────────────────────────────────────────────

void ATDPlayerCharacter::GetCameraAxes(FVector& OutForward, FVector& OutRight) const
{
	// GetForwardVector()는 Roll 영향 없이 카메라 수평 방향 반환 (Roll=90 등 케이스 대응)
	FVector CamForward = CameraComp->GetForwardVector();
	CamForward.Z = 0.f;
	OutForward = CamForward.GetSafeNormal();
	// UE 좌수계: Cross(Up, Forward) = Right
	OutRight = FVector::CrossProduct(FVector::UpVector, OutForward).GetSafeNormal();
}

void ATDPlayerCharacter::HandleCameraMove(const FInputActionValue& Value)
{
	// WASD 입력으로 카메라 패닝
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

	// 마우스가 화면 가장자리에 있으면 카메라 스크롤
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

// ── 클릭 처리 ─────────────────────────────────────────────────────────────────

void ATDPlayerCharacter::HandleClick()
{
	if (!IsLocallyControlled()) return;
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PlayerPawn) return;

	FHitResult HitResult;
	if (!PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult)) return;

	// 타워 설치 가능 지역이면 이동 안 함
	if (HitResult.GetActor() && HitResult.GetActor()->IsA<ATDTowerPawn>()) return;

	PlayerPawn->SetMoveTarget(HitResult.Location);

	// 클릭 위치에 타워 검출 기능은 BP로
}

// ── 기지 체력 ─────────────────────────────────────────────────────────────────

void ATDPlayerCharacter::NotifyBaseHealthDecreased()
{
	// 외부에서 호출 → GameState 체력 감소
	if (ATDGameState* GS = UTDFL_Utility::GetTDGameState(this))
	{
		GS->DecreaseBaseHealth();
	}
}
