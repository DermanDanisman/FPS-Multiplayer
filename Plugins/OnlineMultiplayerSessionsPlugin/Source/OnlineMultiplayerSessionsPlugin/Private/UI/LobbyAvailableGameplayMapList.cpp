// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "UI/LobbyAvailableGameplayMapList.h"

#include "LobbyGameState.h"
#include "LobbyPlayerController.h"
#include "Kismet/GameplayStatics.h"

void ULobbyAvailableGameplayMapList::NativeConstruct()
{
	Super::NativeConstruct();
	
	// Bind to GameState updates so we can react if the Host changes the map.
	ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(UGameplayStatics::GetGameState(GetOwningPlayer()));
	if (!IsValid(LobbyGS)) return;
	
	LobbyGS->OnLobbySettingsUpdatedDelegate.RemoveAll(this); 
	LobbyGS->OnLobbySettingsUpdatedDelegate.AddDynamic(this, &ThisClass::OnLobbySettingsUpdated);
}

void ULobbyAvailableGameplayMapList::UpdateLobbySettings(FLobbySettings NewLobbySettings)
{
	// Send request to Controller (who owns the RPC channel)
	if (ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(GetOwningPlayer()))
	{
		LobbyPC->UpdateLobbySettings(NewLobbySettings);
	}
}