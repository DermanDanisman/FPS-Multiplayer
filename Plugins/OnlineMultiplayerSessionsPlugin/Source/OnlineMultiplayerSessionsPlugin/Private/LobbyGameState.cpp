// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "LobbyGameState.h"
#include "Net/UnrealNetwork.h"

void ALobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, PlayerCount);
	DOREPLIFETIME(ThisClass, LobbySettings);
	
	// COND_Custom: We control this manually in PreReplication.
	DOREPLIFETIME_CONDITION(ThisClass, ChatHistory, COND_Custom);
}

void ALobbyGameState::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);
	
	// OPTIMIZATION:
	// Checks 'bReplicateChatHistory' every network tick.
	// If false, it stops sending the array (saving bandwidth). If true, it resumes.
	DOREPLIFETIME_ACTIVE_OVERRIDE(ThisClass, ChatHistory, bReplicateChatHistory);
}

void ALobbyGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
	
	// Only the Server detects the join first
	if (HasAuthority())
	{
		PlayerCount = PlayerArray.Num();
		OnRep_PlayerCount(); // Force update on Server (so Host UI refreshes)
	}
}

void ALobbyGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
	
	if (HasAuthority())
	{
		PlayerCount = PlayerArray.Num();
		OnRep_PlayerCount(); // Force update on Server
	}
}

void ALobbyGameState::UpdateLobbySettings(const FLobbySettings& NewSettings)
{
	// Only Server can change settings
	if (HasAuthority())
	{
		LobbySettings = NewSettings;
		OnRep_LobbySettings(); // Update Host UI
	}
}

void ALobbyGameState::OnRep_PlayerCount()
{
	// Signal UI: "List is dirty, rebuild it."
	OnPlayerListChangedDelegate.Broadcast();
}

void ALobbyGameState::OnRep_LobbySettings()
{
	// Signal UI: "Map/Mode changed, update text."
	OnLobbySettingsUpdatedDelegate.Broadcast(LobbySettings);
}

void ALobbyGameState::Multicast_BroadcastChatMessage_Implementation(const FChatMessage& NewMessage)
{
	// 1. PERSISTENCE (Server Only)
	// Add to history so future players can see it via Replication.
	if (HasAuthority())
	{
		ChatHistory.Add(NewMessage);
	}

	// 2. LIVE EVENT (Everyone)
	// Fire delegate immediately for "Toast" notifications or sounds.
	OnChatMessageReceived.Broadcast(NewMessage);
}