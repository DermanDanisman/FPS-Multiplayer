// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "PasswordPrompt.h"
#include "ServerSearchResult.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "ServerRow.generated.h"

/**
 * @class UServerRow
 * @brief A single row in the Server Browser List.
 * * INTERFACE: Implements IUserObjectListEntry to work with Unreal's ListView system.
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API UServerRow : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
	
public:
	
	// INTERFACE IMPLEMENTATION:
	// Called automatically by the ListView when it recycles this row and gives it new data.
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	
protected:
	
	// Data Cache: Holds the ViewModel so we can read from it (e.g. Server Name) or use it (Join).
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	TObjectPtr<UServerSearchResult> ServerSearchResult;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Join;
	
	// Config: The class of the password popup widget. Set this in the Editor.
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UPasswordPrompt> PasswordPromptClass;
	
	// --- EVENT HANDLERS ---
	
	UFUNCTION()
	void OnJoinClicked();
	
	// Event Hook: "Data updated, please refresh visual text/colors in Blueprint."
	UFUNCTION(BlueprintImplementableEvent)
	void UpdateVisuals();
	
	// Callback: Fired when the Password Prompt closes.
	UFUNCTION()
	void OnPasswordPromptResult(const FString& EnteredPassword, bool bConfirmed);
	
private:
	// Helper to trigger the Subsystem join call.
	void JoinServer_Internal(const FOnlineSessionSearchResult& Result);
};