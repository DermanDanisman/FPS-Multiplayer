// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "LobbyPlayerState.h"
#include "MenuBaseWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "LobbyMenu.generated.h"

/**
 * @class ULobbyMenu
 * @brief The main UI for the Lobby Level.
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API ULobbyMenu : public UMenuBaseWidget
{
	GENERATED_BODY()
	
public:
	
	virtual void NativeConstruct() override;
	
	// Helper to store the local player's state for easy access.
	UFUNCTION(BlueprintCallable, Category = "Lobby UI")
	void SetPlayerState(ALobbyPlayerState* InPlayerState);
	
	// --- BLUEPRINT IMPLEMENTABLE EVENTS (THE MISSING PIECES) ---
	// These functions exist in C++ so we can call/bind them, but their logic is in Blueprint.
	
	// Fired when PersistentData (Team/Skin) updates.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Lobby UI")
	void OnPersistentDataUpdated(const FPlayerPersistentData& InData);

	// Fired when the Player Count changes (someone joins/leaves). 
	// BP Logic: Clear ScrollBox -> Loop GameState.PlayerArray -> Add Child Widgets.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Lobby UI")
	void OnPlayerListChanged();

	// Fired when *our* local ready status changes (e.g., to play a checkmark animation or sound).
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Lobby UI")
	void OnReadyStatusChanged(bool bIsReady);
	
	// --- INITIALIZATION ---
	// The "Retry" loop that waits for GameState/PlayerState replication.
	void InitLobby();
	
protected:
	
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	TObjectPtr<ALobbyPlayerState> MyPlayerState;
	
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	FLobbySettings CachedLobbySettings;
	
	// --- BUTTONS ---
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Invite;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Ready;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UButton> Button_Start;
	
	// --- LOGIC ---
	UFUNCTION()
	void OnInviteClicked();
	
	UFUNCTION()
	void OnReadyClicked();
	
	UFUNCTION()
	void OnStartClicked();
	
	// Host Only: Checks if all players are ready.
	UFUNCTION(BlueprintCallable, Category = "Lobby Logic")
	void CheckStartButton();
};