// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "OnlineMultiplayerSessionSubsystem.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"

UOnlineMultiplayerSessionSubsystem::UOnlineMultiplayerSessionSubsystem():
	// Constructor: Initialize Delegates. 
	// We bind our internal C++ functions (OnCreateSessionComplete, etc.) to the generic Delegate types.
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete)),
	SessionUserInviteAcceptedDelegate(FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::OnSessionUserInviteAccepted))
{
}

void UOnlineMultiplayerSessionSubsystem::CreateServer(const FServerCreateSettings& Settings)
{
	if (!IsValidSessionInterface()) return;
	
	// IDEMPOTENCY CHECK:
	// If a session named "GameSession" already exists, we cannot create a new one.
	// We flag 'bCreateSessionOnDestroy' = true, destroy the current one, and wait for the callback 
	// to automatically trigger the creation of the new one.
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		bCreateSessionOnDestroy = true;
		LastServerSettings = Settings;
		DestroyServer();
		return;
	}
	
	// Register the callback so we know when Steam finishes creating the session.
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);
	
	// Initialize Settings Object (Heap Allocated via SharedPtr)
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	
	// 1. Basic Configuration
	LastSessionSettings->bIsLANMatch = Settings.bIsLAN;
	LastSessionSettings->NumPublicConnections = Settings.MaxPlayers;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true; // Essential for Steam Friends Join
	LastSessionSettings->bShouldAdvertise = !Settings.bIsPrivate; // If private, hide from Server Browser
	LastSessionSettings->bUsesPresence = true;
	LastSessionSettings->bUseLobbiesIfAvailable = true; // Uses modern Steam Lobbies API
	LastSessionSettings->BuildUniqueId = 1; // Version control (keeps incompatible versions separated)
	
	// 2. Custom Data (Dictionary Keys)
	// These allow the Server Browser to display info without connecting first.
	
	// Server Name
	// We use a fallback if the name is empty to ensure the key always exists.
	FString NameToUse = Settings.ServerName.IsEmpty() ? TEXT("Default Server") : Settings.ServerName;
	LastSessionSettings->Set(SETTING_SERVER_NAME, NameToUse, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	
	// Password Logic
	bool bHasPassword = !Settings.Password.IsEmpty();
	LastSessionSettings->Set(FName("IsPassworded"), bHasPassword, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	if (bHasPassword)
	{
		// Security: Only expose password check via Ping, don't broadcast it in clear text.
		LastSessionSettings->Set(SETTING_SERVER_PASSWORD, Settings.Password, EOnlineDataAdvertisementType::ViaPingOnly);
	}
	
	// Map Image Placeholder (for future use)
	if (Settings.MapImage)
	{
		// Note: We can't replicate a texture directly here. We usually pass a path string.
	}
	
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	
	// TRIGGER: Ask the Online Subsystem to create the session.
	// If it fails immediately (returns false), we clear the delegate and notify UI.
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}
	
}

void UOnlineMultiplayerSessionSubsystem::FindServer(int32 MaxSearchResults)
{
	if (!IsValidSessionInterface()) return;
	
	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);
	
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	
	// CONFIGURATION:
	// AppID 480 (Spacewar) is very noisy. We increase MaxSearchResults to ensure we find *our* lobby 
	// among the thousands of other dev lobbies.
	LastSessionSearch->MaxSearchResults = MaxSearchResults; 

	// FORCE STEAM QUERY:
	// If the subsystem is "NULL" (LAN), bIsLanQuery = true. Else (Steam), false.
	LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL"; 
	
	// CRITICAL: SEARCH LOBBIES
	// "SEARCH_LOBBIES" is a macro for "LOBBYSEARCH".
	// This tells Steam "Look for Lobbies, not Dedicated Servers."
	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	
	// Debug Log
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Subsystem: Starting Search..."));

	// TRIGGER SEARCH
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}

void UOnlineMultiplayerSessionSubsystem::JoinServer(const FOnlineSessionSearchResult& SessionResult)
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}
	
	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	
	// TRIGGER JOIN
	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}

bool UOnlineMultiplayerSessionSubsystem::KickPlayer(const FUniqueNetIdRepl& PlayerId)
{
	if (IsValidSessionInterface())
	{
		// This unregisters them from the OSS Session (Steam backend). 
		// Note: You must ALSO Call GameMode::KickPlayer() to remove their Pawn from the world.
		return SessionInterface->UnregisterPlayer(NAME_GameSession, *PlayerId);
	}
	
	return false;
}

void UOnlineMultiplayerSessionSubsystem::SendInvite(const FUniqueNetIdRepl& FriendId)
{
	if (IsValidSessionInterface())
	{
		const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
		SessionInterface->SendSessionInviteToFriend(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *FriendId);
	}
}

void UOnlineMultiplayerSessionSubsystem::DestroyServer()
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnDestroySessionComplete.Broadcast(false);
		return;
	}

	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		MultiplayerOnDestroySessionComplete.Broadcast(false);
	}
}

void UOnlineMultiplayerSessionSubsystem::StartServer()
{
	if (!IsValidSessionInterface()) return;
	
	StartSessionCompleteDelegateHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegate);
	
	if (!SessionInterface->StartSession(NAME_GameSession))
	{
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
		MultiplayerOnStartSessionComplete.Broadcast(false);
	}
}

bool UOnlineMultiplayerSessionSubsystem::IsValidSessionInterface()
{
	// Lazy Initialization: Get the interface only when needed.
	if (!SessionInterface)
	{
		IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
		if (Subsystem)
		{
			SessionInterface = Subsystem->GetSessionInterface();
		}
	}
	
	return SessionInterface.IsValid();
}

void UOnlineMultiplayerSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	// Cleanup Listener
	if (SessionInterface)
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	// Notify UI
	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void UOnlineMultiplayerSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}

	// VALIDATION:
	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Error, TEXT("FindSessions Failed at the API level (bWasSuccessful=false)"));
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	// RESULT CHECK:
	int32 Count = LastSessionSearch->SearchResults.Num();
	UE_LOG(LogTemp, Warning, TEXT("FindSessions Success. Found %d Results."), Count);

	if (Count <= 0)
	{
		// Success = True, but Results = Empty.
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	// Pass the results to the UI
	MultiplayerOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
}

void UOnlineMultiplayerSessionSubsystem::OnJoinSessionComplete(FName SessionName,
	EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	MultiplayerOnJoinSessionComplete.Broadcast(Result);
}

void UOnlineMultiplayerSessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}
	
	// CHAIN LOGIC:
	// If we were waiting to create a new session (because one already existed), do it now.
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateServer(LastServerSettings); // Recreate with the cached struct
	}
	
	MultiplayerOnDestroySessionComplete.Broadcast(bWasSuccessful);
}

void UOnlineMultiplayerSessionSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
	}
	
	MultiplayerOnStartSessionComplete.Broadcast(bWasSuccessful);
}

void UOnlineMultiplayerSessionSubsystem::OnSessionUserInviteAccepted(const bool bWasSuccessful,
	const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	// 1. Check if the invite is valid
	if (bWasSuccessful)
	{
		if (InviteResult.IsValid())
		{
			// LOG: Visual confirmation for debugging
			if (GEngine) 
			{
				FString DebugMsg = FString::Printf(TEXT("Invite Accepted! Joining: %s"), *InviteResult.Session.OwningUserName);
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, DebugMsg);
			}
            
			// 2. Reuse our existing Join Logic!
			// This avoids duplicating join code.
			JoinServer(InviteResult);
		}
		else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("Invite Accepted but Session was Invalid!"));
		}
	}
}