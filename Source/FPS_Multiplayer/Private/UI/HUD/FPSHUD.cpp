// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "UI/HUD/FPSHUD.h"

#include "Blueprint/UserWidget.h"
#include "UI/Widget/FPSCharacterOverlay.h"
#include "UI/WidgetController/HUD/FPSOverlayController.h"

UFPSOverlayController* AFPSHUD::GetOverlayWidgetController(const FUIWidgetControllerParams& InWidgetControllerParams)
{
	if (OverlayWidgetController == nullptr)
	{
		OverlayWidgetController = NewObject<UFPSOverlayController>(this, OverlayWidgetControllerClass);
		
		OverlayWidgetController->SetWidgetControllerParams(InWidgetControllerParams);
		
		OverlayWidgetController->BindCallbacksToDependencies();
	}
	
	return OverlayWidgetController;
}

void AFPSHUD::InitializeOverlayWidget(APlayerController* InPlayerController, APlayerState* InPlayerState)
{
	// Ensure that the widget and controller classes are configured (usually in BP).
	checkf(OverlayWidgetClass, TEXT("HUD Widget Class uninitialized, please fill out"));
	checkf(OverlayWidgetControllerClass, TEXT("HUD Widget Controller Class uninitialized, please fill out"));
	
	UUserWidget* UserWidget = CreateWidget<UUserWidget>(InPlayerController, OverlayWidgetClass);
	OverlayWidget = Cast<UFPSCharacterOverlay>(UserWidget);
	
	const FUIWidgetControllerParams WidgetControllerParams(InPlayerController, InPlayerState);
	
	UFPSOverlayController* OverlayController = GetOverlayWidgetController(WidgetControllerParams);
	
	OverlayWidget->SetWidgetController(OverlayController);
	
	OverlayWidgetController->BroadcastInitialValues();
	
	UserWidget->AddToPlayerScreen();
}