// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineMultiplayerSessionTypes.h"
#include "UObject/Interface.h"
#include "SeamlessTravelDataInterface.generated.h"

// Boilerplate required by Unreal Reflection System for Interfaces.
UINTERFACE()
class USeamlessTravelDataInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * @class ISeamlessTravelDataInterface
 * @brief The "Mailbox" for receiving persistent data.
 * * USAGE:
 * 1. Implement this interface on your Gameplay PlayerState (MP_PlayerState).
 * 2. In LobbyPlayerState::CopyProperties, check if the target implements this.
 * 3. If yes, call SetPlayerData().
 */
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API ISeamlessTravelDataInterface
{
	GENERATED_BODY()

public:
	
	// The function called by the dying LobbyState to hand off data to the new GameplayState.
	// @param InData: The struct containing TeamId, SkinId, etc.
	virtual void SetPlayerData(const FPlayerPersistentData& InData) = 0;
};