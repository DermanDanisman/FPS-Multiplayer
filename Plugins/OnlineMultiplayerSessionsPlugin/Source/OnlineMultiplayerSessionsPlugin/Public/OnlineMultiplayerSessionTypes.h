// © 2025 Heathrow (Derman Can Danisman). All rights reserved. 
// This project is the intellectual property of Heathrow (Derman Can Danisman).

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OnlineMultiplayerSessionTypes.generated.h"

// --- SESSION KEYS ---
// These Macros act as unique "Dictionary Keys" for the Online Session Settings.
// When we create a server, we "tag" it with these keys.
// When clients search for servers (Server Browser), Steam uses these keys to retrieve this specific data.
#define SETTING_SERVER_NAME FName(TEXT("SERVER_NAME"))
#define SETTING_SERVER_PASSWORD FName(TEXT("SERVER_PASSWORD"))
#define SETTING_MAP_IMAGE FName(TEXT("MAP_IMAGE"))

/**
 * @struct FServerCreateSettings
 * @brief Encapsulates all arguments required to start a server.
 * * DESIGN PATTERN: Parameter Object.
 * Instead of passing 10 separate arguments (Name, MaxPlayers, LAN, Password...) to a function,
 * we bundle them into a single struct. This makes the CreateSession function cleaner 
 * and allows us to add new settings (like "GameMode" or "MapImage") later without breaking existing code.
 */
USTRUCT(BlueprintType)
struct FServerCreateSettings
{
	GENERATED_BODY()

	// The visible name of the server in the browser.
	UPROPERTY(BlueprintReadWrite)
	FString ServerName = "My Cool Lobby";

	// Maximum number of players allowed (including the host).
	UPROPERTY(BlueprintReadWrite)
	int32 MaxPlayers = 4;

	// If true, the server is only visible on the local network, not via Steam.
	UPROPERTY(BlueprintReadWrite)
	bool bIsLAN = false;
	
	// If true, the server will not be advertised via the public matchmaker.
	// Players must join via invite or Friend list.
	UPROPERTY(BlueprintReadWrite)
	bool bIsPrivate = false;

	// The password string. Used to populate the SETTING_SERVER_PASSWORD key.
	UPROPERTY(BlueprintReadWrite)
	FString Password = ""; 

	// The Asset Path to the "Lobby Map". Usually "/Game/Maps/Lobby".
	// We use this path to tell the engine where to travel after the session is created.
	UPROPERTY(BlueprintReadWrite)
	FString MapPath = "/Game/Maps/Lobby"; 
    
	// Visual representation of the selected map (optional).
	UPROPERTY(BlueprintReadWrite)
	UTexture2D* MapImage = nullptr;
};

/**
 * @struct FPlayerPersistentData (The "Backpack")
 * @brief Stores critical player data that must survive level transitions.
 *
 * LIFECYCLE:
 * 1. Main Menu: Stored in GameInstance (PlayerPersistenceSubsystem).
 * 2. Lobby: Copied to LobbyPlayerState.
 * 3. Gameplay: Copied via Interface to GameplayPlayerState.
 */
USTRUCT(BlueprintType)
struct FPlayerPersistentData
{
	GENERATED_BODY()
	
	// 1 = Red, 2 = Blue. 0 = Unassigned.
	UPROPERTY(BlueprintReadWrite, Category = "Data")
	int32 TeamId = 0; 
	
	// ID corresponding to a specific character mesh/material.
	UPROPERTY(BlueprintReadWrite, Category = "Data")
	int32 SkinId = 0;
	
	// Future expansion:
	// UPROPERTY(BlueprintReadWrite)
	// int32 WeaponLoadoutId = 0;
};

/**
 * @struct FLobbySettings
 * @brief Represents the "Live" state of the Lobby (Map, Mode, Difficulty).
 * * NETWORK: This struct is Replicated on the GameState. 
 * When the Host changes a value, it automatically updates on all Clients via OnRep_LobbySettings.
 */
USTRUCT(BlueprintType)
struct FLobbySettings
{
	GENERATED_BODY()

	// The path to the Gameplay Map (Where we go when "Start Game" is clicked).
	UPROPERTY(BlueprintReadWrite)
	FString GameMapPath = "/Game/Maps/GameplayMap";

	// Example: 0 = Easy, 1 = Normal, 2 = Hard
	UPROPERTY(BlueprintReadWrite)
	int32 DifficultyIndex = 0;

	// Example: "Capture the Flag", "Team Deathmatch"
	UPROPERTY(BlueprintReadWrite)
	FString GameModeName = "Deathmatch";
};

// --- CHAT SYSTEM TYPES ---

UENUM(BlueprintType)
enum class EChatMessageType : uint8
{
	Global  UMETA(DisplayName = "Global"),  // Visible to everyone
	Team    UMETA(DisplayName = "Team"),    // Visible only to teammates
	System  UMETA(DisplayName = "System")   // Yellow text for "Player Joined/Left"
};

/**
 * @struct FChatMessage
 * @brief A single line of text in the chat box.
 */
USTRUCT(BlueprintType)
struct FChatMessage
{
	GENERATED_BODY()

	// The display name of the player who sent it.
	UPROPERTY(BlueprintReadOnly)
	FString SenderName;

	// The actual content.
	UPROPERTY(BlueprintReadOnly)
	FString MessageText;

	// Controls the color/visibility logic in the UI.
	UPROPERTY(BlueprintReadOnly)
	EChatMessageType MessageType = EChatMessageType::Global;

	// Optional: Time sent, etc.
};

/**
 * Utility class for defining types exposed to Blueprints.
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API UOnlineMultiplayerSessionTypes : public UObject
{
	GENERATED_BODY()
};