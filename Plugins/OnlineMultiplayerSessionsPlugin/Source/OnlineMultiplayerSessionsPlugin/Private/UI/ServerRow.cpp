// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "UI/ServerRow.h"
#include "OnlineMultiplayerSessionSubsystem.h"

void UServerRow::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	
	// 1. Cast the generic object to our specific ViewModel
	ServerSearchResult = Cast<UServerSearchResult>(ListItemObject);
	
	// 2. Update the UI (Text, Password Lock Icon, etc.)
	if (ServerSearchResult)
	{
		UpdateVisuals(); 
	}
	
	// 3. Bind the Join Button
	if (Button_Join)
	{
		// CRITICAL: Unbind previous delegates! 
		// ListViews recycle widgets. This button might have been bound to a previous server object.
		Button_Join->OnClicked.RemoveAll(this); 
		Button_Join->OnClicked.AddDynamic(this, &ThisClass::OnJoinClicked);
	}
}

void UServerRow::OnJoinClicked()
{
	if (!ServerSearchResult) return;

	// 1. PASSWORD CHECK
	// If the server is locked, interrupt the join process and show the popup.
	if (ServerSearchResult->bHasPassword)
	{
		if (PasswordPromptClass)
		{
			UPasswordPrompt* Prompt = CreateWidget<UPasswordPrompt>(GetWorld(), PasswordPromptClass);
			if (Prompt)
			{
				// Bind to the result delegate so we know what they typed.
				Prompt->OnPasswordResult.AddDynamic(this, &ThisClass::OnPasswordPromptResult);
				Prompt->AddToViewport();
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ServerRow: PasswordPromptClass not set! Cannot join private server."));
		}
		return;
	}

	// 2. NO PASSWORD? JOIN IMMEDIATELY.
	JoinServer_Internal(ServerSearchResult->SearchResult);
}

void UServerRow::OnPasswordPromptResult(const FString& EnteredPassword, bool bConfirmed)
{
	// Did the user click Cancel?
	if (!bConfirmed) return;

	// VERIFY PASSWORD
	// We retrieve the correct password from the hidden session settings metadata.
	FString CorrectPassword;
	ServerSearchResult->SearchResult.Session.SessionSettings.Get(SETTING_SERVER_PASSWORD, CorrectPassword);

	if (EnteredPassword.Equals(CorrectPassword, ESearchCase::CaseSensitive))
	{
		// Password Match: Proceed to Join
		JoinServer_Internal(ServerSearchResult->SearchResult);
	}
	else
	{
		// Password Fail
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Incorrect Password!"));
		if (Button_Join) Button_Join->SetIsEnabled(true); // Re-enable button if we disabled it
	}
}

void UServerRow::JoinServer_Internal(const FOnlineSessionSearchResult& Result)
{
	if (auto* Subsystem = GetGameInstance()->GetSubsystem<UOnlineMultiplayerSessionSubsystem>())
	{
		// UX: Disable button to prevent double-clicking while connecting
		if (Button_Join) Button_Join->SetIsEnabled(false);
		
		Subsystem->JoinServer(Result);
	}
}