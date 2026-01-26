// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "OnlineMultiplayerSessionTypes.h"
#include "GameFramework/PlayerController.h"
#include "LobbyPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UChatComponent;
class ULobbyMenu;

// Delegate: Fired when "Enter" is pressed to open Chat.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnterPressed);

UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API ALobbyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	
	// Called when PlayerState is fully replicated (safe to spawn UI).
	virtual void OnRep_PlayerState() override;
	
	// Helper getter for the Menu Widget.
	UFUNCTION(BlueprintCallable, Category = "Lobby UI")
	FORCEINLINE ULobbyMenu* GetLobbyMenuInstance() { return LobbyMenuInstance; }
	
	// HOST ONLY: Called by UI to change settings.
	UFUNCTION(BlueprintCallable, Category = "Lobby UI")
	void UpdateLobbySettings(const FLobbySettings& NewSettings);
	
	UPROPERTY(BlueprintAssignable, Category = "Input | Delegates")
	FOnEnterPressed OnEnterPressedDelegate;
	
protected:
	
	// --- ENHANCED INPUT ---
	UPROPERTY(EditAnywhere, Category ="Input | Input Mappings")
	TObjectPtr<UInputMappingContext> DefaultMappingContext = nullptr;
	
	UPROPERTY(EditAnywhere, Category="Input | Input Actions")
	TObjectPtr<UInputAction> IA_OpenChat = nullptr;
	
	void OpenChat(const FInputActionValue& InputActionValue);
	
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	
	// Helper to set team data.
	UFUNCTION(BlueprintCallable)
	void PopulatePersistentData(int32 NewTeamId);
	
	// RPC to send settings to server.
	UFUNCTION(Server, Reliable)
	void Server_UpdateLobbySettings(const FLobbySettings& NewSettings);
	
private:
	
	// Widget Class Reference (Set in BP)
	UPROPERTY(EditDefaultsOnly, Category = "Lobby UI")
	TSubclassOf<UUserWidget> LobbyMenuClass;
	
	// Instance Reference
	UPROPERTY()
	TObjectPtr<ULobbyMenu> LobbyMenuInstance;
	
	void TryCreateLobbyMenu();
};