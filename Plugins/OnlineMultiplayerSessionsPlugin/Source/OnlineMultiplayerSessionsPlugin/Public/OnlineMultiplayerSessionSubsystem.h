// © 2025 Heathrow (Derman Can Danisman). All rights reserved. 

#pragma once

#include "CoreMinimal.h"
#include "OnlineMultiplayerSessionTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnlineMultiplayerSessionSubsystem.generated.h"


// --- CUSTOM DELEGATES (Observer Pattern) ---
// The Subsystem performs asynchronous tasks (talking to Steam servers takes time).
// These Delegates fire when those tasks finish, allowing the UI (Loading Screen) to update.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionsComplete, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnSessionInviteReceived, const FString&, InviteSenderName, const FString&, SessionId);

/**
 * @class UOnlineMultiplayerSessionSubsystem
 * @brief The bridge between your Game logic and the Unreal Online Subsystem (Steam/EOS/Null).
 * * LIFECYCLE: Inherits from UGameInstanceSubsystem.
 * This object is created when the game starts and persists across ALL level changes.
 * This is crucial because if we stored this on a GameMode or PlayerController, 
 * the session logic would be destroyed when we traveled between maps.
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API UOnlineMultiplayerSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	
	UOnlineMultiplayerSessionSubsystem();
	
	// --- PUBLIC API (Called by UI) ---

	// 1. Create Server: Configures and hosts a session based on the provided settings struct.
	void CreateServer(const FServerCreateSettings& Settings);

	// 2. Find Server: Queries the backend (Steam) for a list of available sessions.
	// MaxSearchResults: Increases the search pool to handle noisy app IDs (like 480).
	void FindServer(int32 MaxSearchResults);

	// 3. Join Server: Connects the local player to a specific session found in the browser.
	void JoinServer(const FOnlineSessionSearchResult& SessionResult);

	// 4. Kick Player: Deregisters a player from the session (Backend logic only).
	// Note: Does not remove the Pawn; GameMode must handle that separately.
	bool KickPlayer(const FUniqueNetIdRepl& PlayerId);

	// 5. Send Invite: Opens the native platform overlay (e.g. Steam Friends) to send an invite.
	void SendInvite(const FUniqueNetIdRepl& FriendId);

	// Destroys the current session (cleaning up the slot for a new one).
	void DestroyServer();
	
	// Marks the session as "In Progress" (usually removes it from public search results).
	void StartServer();

	// Helper to check if we have a valid connection to the Online Subsystem.
	bool IsValidSessionInterface();

	// --- DELEGATES ---
	// The Menu binds to these to know when async operations complete.
	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionComplete;
	FMultiplayerOnFindSessionsComplete MultiplayerOnFindSessionsComplete;
	FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionComplete;
	FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionComplete;
	FMultiplayerOnStartSessionComplete MultiplayerOnStartSessionComplete;
	
	UPROPERTY(BlueprintAssignable)
	FMultiplayerOnSessionInviteReceived MultiplayerOnSessionInviteReceived;

protected:

	// --- INTERNAL CALLBACKS ---
	// These match the strict signatures required by Unreal's IOnlineSessionInterface.
	// We intercept the Engine's response here, process it, and then fire our Custom Delegates above.
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);

private:
	
	// Pointer to the Engine's Session Interface (The actual worker).
	IOnlineSessionPtr SessionInterface;
	
	// MEMORY SAFETY:
	// We use TSharedPtr to keep these objects alive.
	// If we didn't, the Garbage Collector or Scope could destroy the Search Results before the UI reads them.
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	// --- DELEGATE HANDLES ---
	// We must store these "receipts" so we can unregister our callbacks later.
	// This prevents memory leaks or crashes if the subsystem is destroyed while a callback is pending.
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
	FDelegateHandle StartSessionCompleteDelegateHandle;
	
	FOnSessionUserInviteAcceptedDelegate SessionUserInviteAcceptedDelegate;
	FDelegateHandle SessionUserInviteAcceptedDelegateHandle;

	// LOGIC FLAG:
	// Handles the edge case where a user tries to Create a session while already in one.
	// We must Destroy the old one first, then immediately Create the new one.
	bool bCreateSessionOnDestroy{ false };
	FServerCreateSettings LastServerSettings;
};