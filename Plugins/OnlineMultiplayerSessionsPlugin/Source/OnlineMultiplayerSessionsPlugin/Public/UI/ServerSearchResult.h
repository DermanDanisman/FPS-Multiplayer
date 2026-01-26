// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineMultiplayerSessionTypes.h"
#include "OnlineSessionSettings.h"
#include "Utility/OnlineMultiplayerSessionLibrary.h"
#include "ServerSearchResult.generated.h"

/**
 * @class UServerSearchResult
 * @brief The "ViewModel" for a single server entry.
 * * PROBLEM: FOnlineSessionSearchResult is a raw C++ struct. Blueprints cannot read it directly.
 * * SOLUTION: We wrap it in this UObject. The ServerBrowser creates one of these for each server found,
 * populates it with data, and feeds it to the ListView.
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API UServerSearchResult : public UObject
{
	GENERATED_BODY()
	
public:
	// Internal Data: We keep the original struct so we can pass it back to the "JoinSession" function later.
	FOnlineSessionSearchResult SearchResult;
	
	// SETUP FUNCTION:
	// Called by C++ immediately after creation to extract data from the opaque struct 
	// into readable Blueprint variables.
	void Setup(const FOnlineSessionSearchResult& InResult)
	{
		SearchResult = InResult;

		// 1. Use Library Helpers to parse the Session Settings (Dictionary)
		ServerName     = UOnlineMultiplayerSessionLibrary::GetCustomSessionName(InResult);
		HostName       = UOnlineMultiplayerSessionLibrary::GetHostName(InResult);
		
		// 2. Calculate Slots
		CurrentPlayers = UOnlineMultiplayerSessionLibrary::GetCurrentPlayerCount(InResult);
		MaxPlayers     = SearchResult.Session.SessionSettings.NumPublicConnections;
		
		// 3. Ping
		Ping = SearchResult.PingInMs;
		
		// 4. Password Logic (Check for the existence of the Password Key)
		FString PwdVal;
		if (SearchResult.Session.SessionSettings.Get(SETTING_SERVER_PASSWORD, PwdVal))
		{
			bHasPassword = !PwdVal.IsEmpty();
		}
	}

	// --- BLUEPRINT ACCESSIBLE VARIABLES ---
	// The "ServerRow" widget binds its text blocks to these variables.

	UPROPERTY(BlueprintReadOnly, Category = "Server Info")
	FString ServerName;

	UPROPERTY(BlueprintReadOnly, Category = "Server Info")
	FString HostName;

	UPROPERTY(BlueprintReadOnly, Category = "Server Info")
	int32 CurrentPlayers;

	UPROPERTY(BlueprintReadOnly, Category = "Server Info")
	int32 MaxPlayers;

	UPROPERTY(BlueprintReadOnly, Category = "Server Info")
	int32 Ping;
	
	UPROPERTY(BlueprintReadOnly, Category = "Server Info")
	bool bHasPassword;
};