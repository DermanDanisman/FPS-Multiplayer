// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "OnlineMultiplayerSessionTypes.h"
#include "LobbyGameState.generated.h"

// --- UI DELEGATES ---
// The UI binds to these events. When data changes, we "Broadcast", and the UI updates itself.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerListChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbySettingsUpdated, const FLobbySettings&, NewSettings);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatMessageReceived, const FChatMessage&, Message);

/**
 * @class ALobbyGameState
 * @brief The authoritative state of the Lobby room.
 * * REPLICATION: Replicates to ALL clients.
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API ALobbyGameState : public AGameState
{
	GENERATED_BODY()
	
public:
	
	// REPLICATION OPTIMIZATION:
	// Allows dynamic control over which variables are sent (e.g. Chat History).
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	
	// PLAYER TRACKING:
	// We override these to maintain an accurate 'PlayerCount' for the UI.
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	
	// --- EVENTS ---
	
	// UI Event: Fired when someone joins/leaves so the PlayerList widget can refresh.
	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnPlayerListChanged OnPlayerListChangedDelegate;
	
	// UI Event: Fired when the Host changes a setting.
	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnLobbySettingsUpdated OnLobbySettingsUpdatedDelegate;
	
	// --- SETTINGS ---
	
	// Holds Map/Mode info. ReplicatedUsing ensures clients react immediately.
	UPROPERTY(ReplicatedUsing=OnRep_LobbySettings, BlueprintReadOnly, Category = "Lobby")
	FLobbySettings LobbySettings;
	
	// SERVER ONLY: Called by the Controller to update the struct.
	void UpdateLobbySettings(const FLobbySettings& NewSettings);
	
	// --- CHAT SYSTEM ---
	
	// UI Event: Fired for "Toast" notifications or live chat messages.
	UPROPERTY(BlueprintAssignable, Category = "Chat")
	FOnChatMessageReceived OnChatMessageReceived;
	
	// HISTORY BUFFER:
	// Solves the "Late Joiner" problem. Clients download this array when they connect,
	// allowing them to see messages sent before they finished loading.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Chat")
	TArray<FChatMessage> ChatHistory;
	
	// BROADCAST:
	// Sends a live message to everyone currently connected.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BroadcastChatMessage(const FChatMessage& NewMessage);
	
protected:
	
	// REGISTRY: Registers variables for network replication.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// TRIGGER VARIABLE:
	// We replicate an integer instead of the whole PlayerArray logic. 
	// When this int changes, Clients know to refresh their list.
	UPROPERTY(ReplicatedUsing=OnRep_PlayerCount, BlueprintReadOnly, Category = "Lobby")
	int32 PlayerCount = 0;
	
	// Config: Should we sync history?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby | UI Settings")
	bool bReplicateChatHistory = true;
	
	// --- REACTION FUNCTIONS (OnReps) ---
	UFUNCTION()
	void OnRep_PlayerCount();
	
	UFUNCTION()
	void OnRep_LobbySettings();
};