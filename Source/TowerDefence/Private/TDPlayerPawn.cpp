#include "TDPlayerPawn.h"


ATDPlayerPawn::ATDPlayerPawn()
{
 	PrimaryActorTick.bCanEverTick = true;

}

void ATDPlayerPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

void ATDPlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATDPlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

