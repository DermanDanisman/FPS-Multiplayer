// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "UI/ServerBrowser.h"
#include "OnlineMultiplayerSessionSubsystem.h"
#include "UI/ServerSearchResult.h"

void UServerBrowser::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (Button_Refresh) Button_Refresh->OnClicked.AddDynamic(this, &ThisClass::OnRefreshClicked);
	if (Button_Back) Button_Back->OnClicked.AddDynamic(this, &ThisClass::OnBackClicked);

	// Auto-Search immediately upon opening the menu
	OnRefreshClicked();
}

void UServerBrowser::NativeDestruct()
{
	Super::NativeDestruct();
	// CLEANUP: Ensure we unbind delegates if the widget is destroyed mid-search.
	if (auto* Subsystem = GetGameInstance()->GetSubsystem<UOnlineMultiplayerSessionSubsystem>())
	{
		Subsystem->MultiplayerOnFindSessionsComplete.RemoveAll(this);
	}
}

void UServerBrowser::OnRefreshClicked()
{
	// UX: Disable button to indicate "Working..."
	Button_Refresh->SetIsEnabled(false);
	ServerListView->ClearListItems(); // Wipe old results

	if (auto* Subsystem = GetGameInstance()->GetSubsystem<UOnlineMultiplayerSessionSubsystem>())
	{
		// 1. Bind to the Completion Delegate
		Subsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessionsComplete);
		
		// 2. Start the Search (Async)
		// We request 10,000 results to filter through "Spacewar" noise if testing on AppID 480.
		Subsystem->FindServer(10000);
	}
}

void UServerBrowser::OnFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful)
{
	// 1. Unbind immediately (Single usage)
	if (auto* Subsystem = GetGameInstance()->GetSubsystem<UOnlineMultiplayerSessionSubsystem>())
	{
		Subsystem->MultiplayerOnFindSessionsComplete.RemoveAll(this);
	}

	// 2. Re-enable button
	Button_Refresh->SetIsEnabled(true);

	if (bWasSuccessful)
	{
		// 3. POPULATE THE LIST
		for (const auto& Result : SessionResults)
		{
			// A. Create the ViewModel (UObject wrapper)
			UServerSearchResult* DataObj = NewObject<UServerSearchResult>(this);
			
			// B. Initialize it with the raw data
			DataObj->Setup(Result);

			// C. Feed it to the ListView
			// The ListView automatically spawns a 'ServerRow' widget and calls NativeOnListItemObjectSet.
			ServerListView->AddItem(DataObj);
		}
	}
}

void UServerBrowser::OnBackClicked()
{
	OnServerBrowserBack.Broadcast();
	RemoveFromParent();
}