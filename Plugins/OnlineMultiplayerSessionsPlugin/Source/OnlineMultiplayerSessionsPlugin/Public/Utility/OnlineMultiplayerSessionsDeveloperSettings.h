// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "OnlineMultiplayerSessionsDeveloperSettings.generated.h"

// STRUCT: FMapOption
// Represents a single entry in the "Map Select" dropdown.
USTRUCT(BlueprintType)
struct FMapOption
{
	GENERATED_BODY()

	// The human-readable name (e.g. "Desert Ruins")
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText MapDisplayName;

	// The asset path to the level (e.g. "/Game/Maps/DesertRuins")
	// META: "AllowedClasses" filters the picker to only show World assets.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowedClasses = "/Script/Engine.World"))
	FSoftObjectPath MapReference;

	// Optional: A thumbnail image to display in the UI when this map is selected.
	// Soft Reference prevents loading the texture until we actually need to show it.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> MapImage;
};

/**
 * @class UOnlineMultiplayerSessionsDeveloperSettings
 * @brief Exposes plugin configuration to Project Settings -> Game -> Multiplayer Plugin.
 * * CONFIG: stored in DefaultGame.ini.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Online Multiplayer Sessions Plugin Settings"))
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API UOnlineMultiplayerSessionsDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	
	// The path to the Lobby Map. The plugin needs this to know where to travel after "Create Session".
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Maps", meta = (AllowedClasses = "/Script/Engine.World"))
	FSoftObjectPath LobbyMapPath;

	// A list of playable maps that the Host can select in the Lobby UI.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Maps")
	TArray<FMapOption> AvailableGameplayMaps;
};