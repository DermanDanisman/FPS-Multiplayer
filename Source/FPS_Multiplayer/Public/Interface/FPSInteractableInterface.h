// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FPSInteractableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UFPSInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class FPS_MULTIPLAYER_API IFPSInteractableInterface
{
	GENERATED_BODY()

public:
	// The function generic objects will implement.
	// We pass the "Instigator" (The Player) so the object knows WHO is interacting with it.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(APawn* InstigatorPawn);
	
	// --- NEW: VISUAL FEEDBACK EVENTS ---
	
	// Called when the player looks at / overlaps this object
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction | Visuals")
	void OnFocusGained(APawn* InstigatorPawn);

	// Called when the player looks away / leaves range
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction | Visuals")
	void OnFocusLost(APawn* InstigatorPawn);
};
