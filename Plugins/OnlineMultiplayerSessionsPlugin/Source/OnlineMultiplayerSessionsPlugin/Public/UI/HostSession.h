// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "HostSession.generated.h"

// Delegate to notify parent (MainMenu) to restore its buttons.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHostSessionBack);

/**
 * @class UHostSession
 * @brief Form for configuring a new multiplayer session.
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API UHostSession : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	virtual void NativeConstruct() override;

	// Public Delegate for the parent widget.
	FOnHostSessionBack OnHostSessionBack;
	
protected:
	
	// --- UI ELEMENTS ---
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> Input_ServerName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> Checkbox_Private;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> Input_Password;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Create;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Back;
	
	// --- LOGIC ---
	
	UFUNCTION()
	void OnCreateClicked();

	UFUNCTION()
	void OnBackClicked();
	
	// Validates inputs and updates "Create" button status.
	UFUNCTION()
	void RefreshButtonState();
	
	// UI Events for live updates
	UFUNCTION()
	void OnPrivateFlagChanged(bool bIsChecked);
	
	UFUNCTION()
	void OnServerNameChanged(const FText& Text);
	
	UFUNCTION()
	void OnPasswordTextChanged(const FText& Text);
	
	// Callback from the Subsystem
	UFUNCTION()
	void OnCreateSessionComplete(bool bWasSuccessful);
};