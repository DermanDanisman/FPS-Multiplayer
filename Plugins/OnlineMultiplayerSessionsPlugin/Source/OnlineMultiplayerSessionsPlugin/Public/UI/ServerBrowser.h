// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/ListView.h"
#include "ServerBrowser.generated.h"

// Delegate: "I am done searching, please close me."
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnServerBrowserBack);

/**
 * @class UServerBrowser
 * @brief The Widget containing the search button and the list of servers.
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API UServerBrowser : public UUserWidget
{
	GENERATED_BODY()
public:
	
	FOnServerBrowserBack OnServerBrowserBack;
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// --- UI ELEMENTS ---
	
	// The container that holds the rows.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UListView> ServerListView;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Refresh;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Back;

	// --- LOGIC ---
	
	UFUNCTION()
	void OnRefreshClicked();

	UFUNCTION()
	void OnBackClicked();

	// Callback: Fired when Subsystem finishes searching Steam.
	void OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
};