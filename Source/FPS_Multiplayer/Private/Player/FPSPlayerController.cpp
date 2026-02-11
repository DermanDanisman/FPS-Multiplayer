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
	
	if (IsLocalPlayerController())
	{
		// 3. REGISTER INPUT MAPPING
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
		if (!Subsystem) return;
		
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
		
		if (HUDWidgetClass)
		{
			if (!IsValid(HUDWidgetInstance))
			{
				HUDWidgetInstance = CreateWidget(this, HUDWidgetClass);
			}

			HUDWidgetInstance->AddToPlayerScreen();
		}
	}
}
