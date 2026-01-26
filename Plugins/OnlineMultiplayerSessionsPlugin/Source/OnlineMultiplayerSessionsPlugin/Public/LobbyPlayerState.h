// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineMultiplayerSessionTypes.h"
#include "Component/PlayerIdentityComponent.h"
#include "GameFramework/PlayerState.h"
#include "LobbyPlayerState.generated.h"

class UChatComponent;

// --- DELEGATES (Observer Pattern) ---
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyPlayerStateReadyChanged, bool, bIsReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPersistentDataSet, const FPlayerPersistentData&, NewPersistentData);

/**
 * @class ALobbyPlayerState
 * @brief The "Data Container" for a player in the Lobby.
 * * RESPONSIBILITY:
 * 1. Store Team/Skin info (PersistenceData).
 * 2. Handle the "Ready" checkmark logic.
 * 3. Act as the "Bridge" to carry data into the Gameplay Map (via CopyProperties).
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API ALobbyPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	
	ALobbyPlayerState();
	
	// NETWORK REGISTRY: Registers variables to be synchronized to clients.
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	// --- UI INTERFACE (Setters) ---
	// Called by WBP_Lobby to change settings. 
	// These functions handle the "Client -> Server" RPC logic automatically.

	UFUNCTION(BlueprintCallable, Category = "Lobby|Gameplay")
	void SetTeam(int32 NewTeamId);

	UFUNCTION(BlueprintCallable, Category = "Lobby|Cosmetics")
	void SetSkin(int32 NewSkinId);
	
	// The "Backpack": Stores the Team ID and Skin ID.
	// ReplicatedUsing = OnRep_PersistenceData: Calls the function on Clients when data changes.
	UPROPERTY(ReplicatedUsing=OnRep_PersistenceData, BlueprintReadOnly, Category = "Lobby | Data")
	FPlayerPersistentData PersistenceData;
	
	UPROPERTY(BlueprintAssignable, Category = "Lobby | Data")
	FOnPersistentDataSet OnPersistentDataSet;
	
	// --- READY LOGIC ---
	
	// Called by the "Ready" button in the UI.
	UFUNCTION(BlueprintCallable, Category="Lobby")
	void SetIsReady(bool bReady); 
	
	// RPC: Sends the "Ready" state to the Server.
	UFUNCTION(Server, Reliable)
	void Server_SetIsReady(bool bReady);
	
	// Callback: Runs on Clients when the Server updates 'bIsReady'.
	UFUNCTION()
	void OnRep_IsReady();
	
	UPROPERTY(ReplicatedUsing = OnRep_IsReady, BlueprintReadOnly, Category = "Lobby")
	bool bIsReady = false;
	
	UPROPERTY(BlueprintAssignable)
	FOnLobbyPlayerStateReadyChanged OnReadyChanged;
	
	// --- COMPONENTS ---
	
	// Handles Steam Avatar fetching.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPlayerIdentityComponent> IdentityComponent;
	
	// Handles Chat messaging.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UChatComponent> ChatComponent;

protected:
	
	virtual void BeginPlay() override;
	
	// --- SEAMLESS TRAVEL BRIDGE ---
	// CRITICAL: This is the bridge function.
	// Called during map transition to copy data from this OLD state to the NEW state.
	virtual void CopyProperties(APlayerState* PlayerState) override;
	
	// --- INTERNAL SERVER LOGIC ---
	
	UFUNCTION(Server, Reliable)
	void Server_SetTeam(int32 NewTeamId);

	UFUNCTION(Server, Reliable)
	void Server_SetSkin(int32 NewSkinId);
	
	UFUNCTION()
	void OnRep_PersistenceData();
};