// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "LobbyGameMode.h"

#include "LobbyGameState.h"
#include "LobbyPlayerController.h"
#include "LobbyPlayerState.h"
#include "OnlineMultiplayerSessionSubsystem.h"

ALobbyGameMode::ALobbyGameMode()
{
	// 1. ENABLE SEAMLESS TRAVEL
	// Critical for multiplayer. Without this, disconnecting from the Lobby Map would 
	// disconnect all clients before they load into the Game Map.
	bUseSeamlessTravel = true;
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	ALobbyPlayerState* LobbyPS = NewPlayer->GetPlayerState<ALobbyPlayerState>();
	if (!LobbyPS) return;

	// --- BLOCK 1: HOST SPECIFIC LOGIC ---
	// "IsLocalController" is TRUE only for the Listen-Server Host.
	if (NewPlayer->IsLocalController())
	{
		// Convenience: Host is always ready.
		LobbyPS->bIsReady = true;
	}

	// --- BLOCK 2: GLOBAL NOTIFICATIONS ---
	// ENGINEERING FIX: 
	// We moved this block OUTSIDE the 'IsLocalController' check.
	// Previously, this was inside, causing only the Host to trigger "Player Joined" messages.
	// Now, it runs for every single player (Client or Host) that joins.
	if (ALobbyGameState* LobbyGS = GetGameState<ALobbyGameState>())
	{
		FString PlayerName = LobbyPS->GetPlayerName();
		if (PlayerName.IsEmpty()) PlayerName = "New Player"; // Fallback if Steam hasn't loaded name
        
		FString Msg = FString::Printf(TEXT("%s has joined the lobby."), *PlayerName);

		// Send System Message to the GameState (which handles the broadcast)
		LobbyGS->Multicast_BroadcastChatMessage(
			FChatMessage{ "System", Msg, EChatMessageType::System }
		);
	}
}

void ALobbyGameMode::StartGame()
{
	// 1. UPDATE SUBSYSTEM STATUS
	// Tell Steam/OnlineServices that the session is now "In Progress" (removes from browser usually).
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		if (auto* Subsystem = GI->GetSubsystem<UOnlineMultiplayerSessionSubsystem>())
		{
			Subsystem->StartServer();
		}
	}

	// 2. RESOLVE MAP PATH
	// We use SoftObjectPath to convert the specific asset path to a Package Name.
	FString TargetMap = "/Game/Maps/GameplayMap"; // Default fallback
	
	// Get the selected map from the GameState (Host's selection)
	const FLobbySettings& Settings = GetGameState<ALobbyGameState>()->LobbySettings;
	
	if (!Settings.GameMapPath.IsEmpty())
	{
		// FIX: Convert "/Game/Maps/Map1.Map1" -> "/Game/Maps/Map1"
		TargetMap = FSoftObjectPath(Settings.GameMapPath).GetLongPackageName();
	}

	// 3. SERVER TRAVEL
	// Must be called with "?listen" to maintain the server state.
	UWorld* World = GetWorld();
	if (World)
	{
		UE_LOG(LogTemp, Warning, TEXT("LobbyGameMode: Seamless Travel to %s"), *TargetMap);
		World->ServerTravel(FString::Printf(TEXT("%s?listen"), *TargetMap));
	}
}