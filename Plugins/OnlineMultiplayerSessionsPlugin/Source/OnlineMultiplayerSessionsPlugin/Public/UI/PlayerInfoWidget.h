// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "LobbyPlayerState.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "PlayerInfoWidget.generated.h"

/**
 * @class UPlayerInfoWidget
 * @brief Modular widget for displaying Player Name + Avatar.
 * * USAGE: Can be used in 2D Menus (Lobby) or 3D World (Overhead Health Bar).
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API UPlayerInfoWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
	
public:
	
	// --- SETUP ---
	
	// Main Initialization function.
	// @param InPlayerState: The player to display.
	// @param bIs3DWorldWidget: Changes visual behavior (e.g. visibility distance) if true.
	UFUNCTION(BlueprintCallable, Category = "UI")
	virtual void SetPlayerState(APlayerState* InPlayerState, bool bIs3DWorldWidget = false);
	
protected:
	
	// The player we are watching.
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	TObjectPtr<APlayerState> MyPlayerState;

	UPROPERTY(BlueprintReadOnly, Category = "Config")
	bool bIsIn3DWorld = false;

	// --- VISUAL UPDATES ---
	
	// Event Hook: "Data Changed". Implemented in Blueprint to update Text/Image widgets.
	UFUNCTION(BlueprintImplementableEvent)
	void UpdateVisuals();

	// Callback: Fired when the Steam Avatar finishes downloading.
	UFUNCTION()
	void OnAvatarLoaded(UTexture2D* AvatarTexture);
};