// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPSUserWidget.h"
#include "Blueprint/UserWidget.h"
#include "FPSCharacterOverlay.generated.h"


/**
 * The C++ Parent for our HUD Widget.
 * We bind these variables to the Designer view in the Editor.
 */
UCLASS()
class FPS_MULTIPLAYER_API UFPSCharacterOverlay : public UFPSUserWidget
{
	GENERATED_BODY()
	
public:
	// --- AMMO ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> WeaponOverlay;

	// --- HEALTH ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> HealthOverlay;
};
