// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineMultiplayerSessionTypes.h"
#include "Blueprint/UserWidget.h"
#include "LobbyAvailableGameplayMapList.generated.h"

/**
 * @class ULobbyAvailableGameplayMapList
 * @brief Widget for selecting the Gameplay Map (ComboBox).
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API ULobbyAvailableGameplayMapList : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	virtual void NativeConstruct() override;
	
protected:
	
	// Event (From GameState): "Host changed map, update selection."
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OnLobbySettingsUpdated(const FLobbySettings& InLobbySettings);
	
	// Function (From UI): "I selected a map, tell the server."
	UFUNCTION(BlueprintCallable, Category = "Lobby UI")
	void UpdateLobbySettings(FLobbySettings NewLobbySettings);
};