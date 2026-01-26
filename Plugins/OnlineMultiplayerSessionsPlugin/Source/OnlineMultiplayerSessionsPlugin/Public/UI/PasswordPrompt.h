// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "PasswordPrompt.generated.h"

// DELEGATE: Sends back (PasswordString, bConfirmed)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPasswordResult, const FString&, Password, bool, bConfirmed);

UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API UPasswordPrompt : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	FOnPasswordResult OnPasswordResult;
	
protected:
	
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> Input_Password;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Join;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Cancel;

	UFUNCTION()
	void OnJoinClicked();

	UFUNCTION()
	void OnCancelClicked();
};