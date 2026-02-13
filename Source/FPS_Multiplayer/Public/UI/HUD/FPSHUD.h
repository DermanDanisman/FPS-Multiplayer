// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FPSHUD.generated.h"

// Forward Declarations
class UFPSCombatComponent;
struct FUIWidgetControllerParams;
class UFPSUserWidget;
class UFPSOverlayController;

/**
 * 
 */
UCLASS()
class FPS_MULTIPLAYER_API AFPSHUD : public AHUD
{
	GENERATED_BODY()
	
// =========================================================================
//                        UMG WIDGETS
// =========================================================================
	
public:
	
	/**
	 * InitializeHUD
	 * Spawns the HUD widget, creates/initializes its controller, and adds the widget to the viewport.
	 */
	void InitializeOverlayWidget(APlayerController* InPlayerController, APlayerState* InPlayerState);
	
	/**
	 * GetHUDWidgetController
	 * Returns a pointer to the HUD Widget Controller, creating and initializing it if none exists.
	 * @param InWidgetControllerParams   References required by the widget controller.
	 */
	UFPSOverlayController* GetOverlayWidgetController(const FUIWidgetControllerParams& InWidgetControllerParams);

private:
	
    // ===== Widget instances =====

    /** On-screen HUD widget instance (your custom user widget). */
    UPROPERTY()
    TObjectPtr<UFPSUserWidget> OverlayWidget;

    /** Concrete class used to spawn the HUD widget at runtime (set in BP). */
    UPROPERTY(EditAnywhere, Category="UI|Classes")
    TSubclassOf<UUserWidget> OverlayWidgetClass;

    /** HUD widget controller instance. Owned by this HUD. */
    UPROPERTY()
    TObjectPtr<UFPSOverlayController> OverlayWidgetController;

    /** Concrete class used to instantiate the HUD widget controller (set in BP). */
    UPROPERTY(EditAnywhere, Category="UI|Classes")
    TSubclassOf<UFPSOverlayController> OverlayWidgetControllerClass;
};
