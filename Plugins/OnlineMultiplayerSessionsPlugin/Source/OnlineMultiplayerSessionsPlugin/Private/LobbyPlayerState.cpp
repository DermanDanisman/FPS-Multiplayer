// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "LobbyPlayerState.h"
#include "Component/ChatComponent.h"
#include "Component/PlayerIdentityComponent.h"
#include "Interface/SeamlessTravelDataInterface.h"
#include "Net/UnrealNetwork.h"

ALobbyPlayerState::ALobbyPlayerState()
{
	// Modular Design: Logic is split into Components to keep this class clean.
	IdentityComponent = CreateDefaultSubobject<UPlayerIdentityComponent>(TEXT("IdentityComponent"));
	ChatComponent = CreateDefaultSubobject<UChatComponent>("ChatComponent");
	
	// CRITICAL: Components on PlayerState MUST replicate to allow Server->Client RPCs.
	ChatComponent->SetIsReplicated(true);
}

void ALobbyPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, bIsReady);
	
	// REPNOTIFY_Always: 
	// Ensures OnRep fires even if we set the value to the SAME value (e.g. forcing a UI refresh).
	DOREPLIFETIME_CONDITION_NOTIFY(ALobbyPlayerState, PersistenceData, COND_None, REPNOTIFY_Always);
}

void ALobbyPlayerState::BeginPlay()
{
	Super::BeginPlay();
	
	// Auto-fetch Steam Avatar when this player spawns.
	if (IdentityComponent)
	{
		IdentityComponent->FetchSteamAvatar();
	}
}

void ALobbyPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	
	// --- SEAMLESS TRAVEL BRIDGE ---
	// 'PlayerState' here is the NEW, empty state in the Gameplay Map.
	
	// INTERFACE CHECK: 
	// We check if the destination actor implements our "Data Receiver" interface.
	if (ISeamlessTravelDataInterface* Interface = Cast<ISeamlessTravelDataInterface>(PlayerState))
	{
		// Pass the backpack safely
		Interface->SetPlayerData(PersistenceData);
		UE_LOG(LogTemp, Warning, TEXT("[STEP 3] LobbyPS: Handed data to Interface. Team: %d"), PersistenceData.TeamId);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("LobbyPlayerState: FAILED! New PlayerState does not implement ISeamlessTravelDataInterface!"));
	}
}

void ALobbyPlayerState::SetTeam(int32 NewTeamId)
{
	// PROXY CHECK:
	// If I am the Server, do it. If I am a Client, ask the Server.
	if (HasAuthority()) Server_SetTeam_Implementation(NewTeamId);
	else Server_SetTeam(NewTeamId);
}

void ALobbyPlayerState::Server_SetTeam_Implementation(int32 NewTeamId)
{
	PersistenceData.TeamId = NewTeamId;
	ForceNetUpdate(); // Send this change NOW.
	OnRep_PersistenceData(); // Manually update Server UI.
}

void ALobbyPlayerState::SetSkin(int32 NewSkinId)
{
	if (HasAuthority()) Server_SetSkin_Implementation(NewSkinId);
	else Server_SetSkin(NewSkinId);
}

void ALobbyPlayerState::Server_SetSkin_Implementation(int32 NewSkinId)
{
	PersistenceData.SkinId = NewSkinId;
	ForceNetUpdate();
	OnRep_PersistenceData();
}

void ALobbyPlayerState::OnRep_PersistenceData()
{
	// Debug Log
	UE_LOG(LogTemp, Error, TEXT("DATA STATE -> Team: %d | Skin: %d"), PersistenceData.TeamId, PersistenceData.SkinId);
	OnPersistentDataSet.Broadcast(PersistenceData);
}

void ALobbyPlayerState::SetIsReady(bool bReady)
{
	if (!HasAuthority())
	{
		Server_SetIsReady(bReady); 
		return;
	}
	
	bIsReady = bReady;
	OnRep_IsReady(); // Manual call for Host
}

void ALobbyPlayerState::Server_SetIsReady_Implementation(bool bReady)
{
	bIsReady = bReady;
	OnRep_IsReady();
}

void ALobbyPlayerState::OnRep_IsReady()
{
	OnReadyChanged.Broadcast(bIsReady);
}