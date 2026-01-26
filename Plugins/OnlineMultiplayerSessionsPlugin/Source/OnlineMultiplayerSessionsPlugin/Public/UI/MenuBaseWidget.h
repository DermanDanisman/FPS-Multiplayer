// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuBaseWidget.generated.h"

/**
 * @class UMenuBaseWidget
 * @brief Base class for any "Full Screen" menu (Main Menu, Lobby, Pause).
 * * PURPOSE: Handles the boilerplate of Input Modes, Mouse Visibility, and Focus.
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API UMenuBaseWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	// Sets Input Mode to UI Only, Shows Mouse, Focuses Widget.
	UFUNCTION(BlueprintCallable, Category = "Menu Logic")
	virtual void MenuSetup();

	// Sets Input Mode to Game Only, Hides Mouse.
	UFUNCTION(BlueprintCallable, Category = "Menu Logic")
	virtual void MenuTearDown();
	
protected:
	virtual void NativeDestruct() override;
};
