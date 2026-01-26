// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineMultiplayerSessionTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PlayerPersistenceSubsystem.generated.h"

/**
 * @class UPlayerPersistenceSubsystem
 * @brief "The Vault"
 * * PROBLEM: When traveling from "MainMenu" -> "Lobby", the PlayerController is destroyed.
 * Any skin selection made in the Main Menu is lost.
 * * SOLUTION: We store that data here (GameInstance Subsystem). When the new Lobby PlayerController 
 * spawns, it retrieves the data from here.
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API UPlayerPersistenceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	// The "Backpack" contents.
	// Stores TeamId, SkinId, etc.
	UPROPERTY(BlueprintReadWrite, Category = "Persistence")
	FPlayerPersistentData LocalPlayerData;
	
	// Helper function for Blueprints to quickly save a Skin choice.
	UFUNCTION(BlueprintCallable, Category = "Persistence")
	void SetLocalSkin(int32 SkinId)
	{
		LocalPlayerData.SkinId = SkinId;
	}
};