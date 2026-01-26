// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "UI/LobbyPlayerRow.h"
#include "LobbyPlayerState.h"

void ULobbyPlayerRow::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	// 1. Call Parent (ListView boilerplate)
	Super::NativeOnListItemObjectSet(ListItemObject);
	
	// 2. Cast and Setup
	if (ALobbyPlayerState* CastedState = Cast<ALobbyPlayerState>(ListItemObject))
	{
		SetPlayerState(CastedState, false);
	}
}

void ULobbyPlayerRow::SetPlayerState(APlayerState* InPlayerState, bool bIs3DWorldWidget)
{
	// 1. Parent Setup (Name & Avatar)
	Super::SetPlayerState(InPlayerState, bIs3DWorldWidget);

	// 2. Lobby Specifics (Ready Status)
	if (ALobbyPlayerState* LobbyPS = Cast<ALobbyPlayerState>(InPlayerState))
	{
		// Clean Binding
		LobbyPS->OnReadyChanged.RemoveAll(this);
		LobbyPS->OnReadyChanged.AddDynamic(this, &ThisClass::OnReadyStatusChanged);

		// Initial Sync
		OnReadyStatusChanged(LobbyPS->bIsReady);
	}
}

void ULobbyPlayerRow::OnReadyStatusChanged(bool bIsReady)
{
	// Calls Blueprint Implementable Event in Parent to update visuals
	UpdateVisuals();
}