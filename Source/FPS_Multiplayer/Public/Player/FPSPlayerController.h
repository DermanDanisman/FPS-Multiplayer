// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FPSPlayerController.generated.h"

class UInputMappingContext;
/**
 * 
 */
UCLASS()
class FPS_MULTIPLAYER_API AFPSPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	
	AFPSPlayerController();
	
protected:
	
	virtual void OnPossess(APawn* InPawn) override;
	virtual void BeginPlay() override;
	
	// --- ENHANCED INPUT CONFIGURATION ---
	
	// The "Map" of keys. (e.g. WASD -> Move, Mouse -> Look)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
private:
	
	UPROPERTY(EditDefaultsOnly, Category = "UI | HUD")
	TSubclassOf<UUserWidget> HUDWidgetClass;
	
	UPROPERTY()
	TObjectPtr<UUserWidget> HUDWidgetInstance;
};
