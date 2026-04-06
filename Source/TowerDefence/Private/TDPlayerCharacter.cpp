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

ATDPlayerCharacter::ATDPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 루트: 카메라 앵커 전용
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
	SetRootComponent(Root);

	// SpringArm
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->TargetArmLength = 600.f;
	SpringArmComp->SetRelativeRotation(FRotator(0.0f, -50.f, 90.f));
	SpringArmComp->bUsePawnControlRotation = false;
	SpringArmComp->bInheritPitch  = false;
	SpringArmComp->bInheritYaw    = false;
	SpringArmComp->bInheritRoll   = false;
	SpringArmComp->bDoCollisionTest = false;

	// Camera
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;

}

void ATDPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;

		if (UEnhancedInputLocalPlayerSubsystem* Sub =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Sub->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// 플레이어 폰 스폰 (카메라 앵커와 같은 위치에서 시작)
	if (PlayerPawnClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		PlayerPawn = GetWorld()->SpawnActor<ATDPlayerPawn>(PlayerPawnClass, GetActorLocation(), FRotator::ZeroRotator, Params);
	}

	if(HUDClass)
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
		EIC->BindAction(MoveAction,  ETriggerEvent::Triggered, this, &ATDPlayerCharacter::HandleCameraMove);
		EIC->BindAction(ClickAction, ETriggerEvent::Started,   this, &ATDPlayerCharacter::HandleClick);
	}
}

void ATDPlayerCharacter::TickEdgeScroll(float DeltaTime)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	int32 VX, VY;
	PC->GetViewportSize(VX, VY);
	float MX, MY;
	if (!PC->GetMousePosition(MX, MY)) return;

	FVector2D Dir = FVector2D::ZeroVector;
	if      (MX < EdgeScrollThreshold)          Dir.X = -1.f;
	else if (MX > VX - EdgeScrollThreshold)     Dir.X =  1.f;
	if      (MY < EdgeScrollThreshold)          Dir.Y =  1.f;
	else if (MY > VY - EdgeScrollThreshold)     Dir.Y = -1.f;

	if (!Dir.IsZero())
	{
		// Camera RightVector 기준으로 수평 이동 방향 계산 (Roll/Yaw 무관)
		FVector Right = CameraComp->GetRightVector();
		Right.Z = 0.f;
		Right = Right.GetSafeNormal();
		FVector Forward = FVector::CrossProduct(Right, FVector::UpVector).GetSafeNormal();
		SetActorLocation(GetActorLocation() + (Forward * Dir.Y + Right * Dir.X) * EdgeScrollSpeed * DeltaTime);
	}
}

void ATDPlayerCharacter::HandleCameraMove(const FInputActionValue& Value)
{
	const FVector2D V = Value.Get<FVector2D>();
	FVector Right = CameraComp->GetRightVector();
	Right.Z = 0.f;
	Right = Right.GetSafeNormal();
	FVector Forward = FVector::CrossProduct(Right, FVector::UpVector).GetSafeNormal();
	SetActorLocation(GetActorLocation() + (Forward * V.Y + Right * V.X) * EdgeScrollSpeed * GetWorld()->GetDeltaSeconds());
}

void ATDPlayerCharacter::NotifyBaseHealthDecreased()
{
	if (ATDGameState* GS = UTDFL_Utility::GetTDGameState(this))
	{
		GS->DecreaseBaseHealth();
	}
}

void ATDPlayerCharacter::HandleClick()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PlayerPawn) return;

	FHitResult HitResult;
	if (!PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult)) return;

	// 타워 설치 가능 지역이면 이동 안 함
	if (HitResult.GetActor() && HitResult.GetActor()->IsA<ATDTowerPawn>()) return;

	PlayerPawn->SetMoveTarget(HitResult.Location);

	// 클릭위치에 타워 검출 기능은 BP로
}
