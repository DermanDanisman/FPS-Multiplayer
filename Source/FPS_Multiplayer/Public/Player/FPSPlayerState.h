// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "FPSPlayerState.generated.h"

class UFPSChatComponent;
class UFPSPlayerIdentityComponent;

UCLASS()
class FPS_MULTIPLAYER_API AFPSPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	
	AFPSPlayerState();
	
	// --- COMPONENTS ---
	
	// Handles Steam Avatar fetching.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UFPSPlayerIdentityComponent> IdentityComponent;
	
	// Handles Chat messaging.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UFPSChatComponent> ChatComponent;
	
protected:
	
	virtual void BeginPlay() override;
};
