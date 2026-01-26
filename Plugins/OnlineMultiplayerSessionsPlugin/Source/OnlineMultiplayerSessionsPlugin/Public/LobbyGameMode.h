// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

/**
 * @class ALobbyGameMode
 * @brief The Authority Logic for the Lobby Level.
 * * EXISTENCE: Server Only. Clients do NOT have this actor.
 * * RESPONSIBILITY: 
 * 1. Handling Player Login/Logout (PostLogin).
 * 2. Managing the transition to the Gameplay Map (ServerTravel).
 * 3. Enforcing "Seamless Travel" to keep connections alive.
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	
	ALobbyGameMode();
	
	// --- LIFECYCLE ---

	// Called immediately after a player successfully joins the session.
	// This is our hook to:
	// 1. Initialize their data (if needed).
	// 2. Broadcast the "Player Joined" notification.
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
	// --- GAMEPLAY API ---
	
	// Called by the Host's "Start Game" button in the UI.
	// Initiates the Seamless Travel to the selected gameplay map.
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void StartGame();
};