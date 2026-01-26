// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MenuBaseWidget.h"

void UMenuBaseWidget::MenuSetup()
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			
			PC->SetInputMode(InputModeData);
			PC->SetShowMouseCursor(true);
		}
	}
}

void UMenuBaseWidget::MenuTearDown()
{
	RemoveFromParent();

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			FInputModeGameOnly InputModeData;
			PC->SetInputMode(InputModeData);
			PC->SetShowMouseCursor(false);
		}
	}
}

void UMenuBaseWidget::NativeDestruct()
{
	// Safety: Ensure input is restored if the widget is destroyed by the engine (e.g. level change)
	MenuTearDown();
	Super::NativeDestruct();
}