// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "Player/FPSPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"

AFPSPlayerController::AFPSPlayerController()
{
	bReplicates = true;
}

void AFPSPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (InPawn->IsLocallyControlled())
	{
		
	}
}

void AFPSPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
}
