// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "PlayerInfoWidget.h"
#include "LobbyPlayerRow.generated.h"

/**
 * @class ULobbyPlayerRow
 * @brief Extends PlayerInfoWidget to add "Ready" status for the Lobby List.
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API ULobbyPlayerRow : public UPlayerInfoWidget
{
	GENERATED_BODY()

public:
	
	// Called by ListView when data is assigned.
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	
	// Override parent setup to also bind Ready Status.
	virtual void SetPlayerState(APlayerState* InPlayerState, bool bIs3DWorldWidget) override;
	
protected:
	
	// Logic to show/hide the Green Checkmark.
	UFUNCTION()
	void OnReadyStatusChanged(bool bIsReady);
};