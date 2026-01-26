// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineMultiplayerSessionsDeveloperSettings.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameFramework/OnlineReplStructs.h" // For FUniqueNetIdRepl
#include "OnlineMultiplayerSessionLibrary.generated.h"

// Enum for avatar resolution requests
UENUM(BlueprintType)
enum class ESteamAvatarSize : uint8
{
	Small   UMETA(DisplayName = "Small (32x32)"),
	Medium  UMETA(DisplayName = "Medium (64x64)"),
	Large   UMETA(DisplayName = "Large (184x184)")
};

/**
 * @class UOnlineMultiplayerSessionLibrary
 * @brief Static Utility Class.
 * * PURPOSE: Functions here can be called from anywhere (Blueprints/C++) without an object instance.
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API UOnlineMultiplayerSessionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	// --- AVATAR FUNCTIONS ---
	
	// Takes a UniqueNetId (from PlayerState), asks Steam for the image, and returns a UTexture2D.
	// Returns nullptr if the image isn't ready yet (Steam downloads are async).
	UFUNCTION(BlueprintCallable, Category = "Steam|Avatars")
	static UTexture2D* GetSteamAvatar(const FUniqueNetIdRepl& UniqueId, ESteamAvatarSize Size = ESteamAvatarSize::Medium);
	
	// Helper to get the local user's avatar.
	UFUNCTION(BlueprintCallable, Category = "Steam|Avatars")
	static UTexture2D* GetLocalSteamAvatar(ESteamAvatarSize Size = ESteamAvatarSize::Medium);
	
	// Opens the Steam Overlay (Shift+Tab) to the Friends tab.
	UFUNCTION(BlueprintCallable, Category = "Steam|UI")
	static void OpenSteamFriendsUI();
	
	// --- SESSION SEARCH HELPERS ---
	// "FOnlineSessionSearchResult" is an opaque struct. These helpers extract readable data.

	// Extracts the "SERVER_NAME" key we set in the Subsystem.
	static FString GetCustomSessionName(const FOnlineSessionSearchResult& SearchResult);

	// Calculates: (MaxPublic + MaxPrivate) - (OpenPublic + OpenPrivate)
	static int32 GetCurrentPlayerCount(const FOnlineSessionSearchResult& SearchResult);

	// Gets the owning username (Host)
	static FString GetHostName(const FOnlineSessionSearchResult& SearchResult);

	// Pings Google to check if we have internet (useful for disabling multiplayer buttons).
	UFUNCTION(BlueprintCallable, Category = "Multiplayer|Network")
	static void CheckInternetConnection(const FString& TestURL = "https://www.google.com");
	
	// Accessor for the Developer Settings.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Multiplayer|Settings")
	static const UOnlineMultiplayerSessionsDeveloperSettings* GetOnlineMultiplayerSessionsDeveloperSettings();
	
	// UTILITY: Forces a Soft Object Reference to load into memory immediately.
	UFUNCTION(BlueprintPure, Category = "Multiplayer|Helpers")
	static UTexture2D* LoadSoftTexture(TSoftObjectPtr<UTexture2D> SoftTexture);
};