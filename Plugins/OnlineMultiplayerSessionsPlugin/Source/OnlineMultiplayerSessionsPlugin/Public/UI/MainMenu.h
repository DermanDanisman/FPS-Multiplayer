// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MenuBaseWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "MainMenu.generated.h"

class UPanelWidget; // Forward declaration reduces compile time

/**
 * @class UMainMenu
 * @brief The Root Widget for the Entry Level.
 * * RESPONSIBILITY:
 * 1. Display the primary options (Host, Find, Quit).
 * 2. Handle the "View Stack" by injecting sub-menus (HostSession, ServerBrowser) into a container slot.
 * 3. Manage visibility of main buttons when a sub-menu is active.
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API UMainMenu : public UMenuBaseWidget
{
	GENERATED_BODY()
	
protected:
	
	// LIFECYCLE: Called when the widget is created and added to the viewport.
	// We use this instead of "Construct" to ensure C++ bindings are ready.
	virtual void NativeConstruct() override;
	
	// --- UI BINDINGS (BindWidget) ---
	// Pointers must match the Variable Names in the Widget Blueprint exactly.
	// If they don't match, the game will crash or ensure() on load.
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Host;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Search;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Quit;
	
	// --- CONTAINER ---
	// This acts as a "Placeholder Frame". We inject sub-widgets here.
	// In Blueprint, this is usually a SizeBox, VerticalBox, or Overlay.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> ServerBrowserContainer;
	
	// --- CONFIGURATION ---
	// References to the sub-widget classes to spawn.
	// Set these in the Editor Details Panel of WBP_MainMenu.
	
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	TSubclassOf<UUserWidget> ServerBrowserClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	TSubclassOf<UUserWidget> HostSessionClass;

	// --- EVENT HANDLERS ---
	// Bound to OnClicked events in NativeConstruct.
	
	UFUNCTION()
	void OnHostClicked();

	UFUNCTION()
	void OnSearchClicked();

	UFUNCTION()
	void OnQuitClicked();
	
	// Callback: Fired when a child widget (HostSession/ServerBrowser) wants to go back.
	UFUNCTION()
	void OnChildWidgetClosed();
};