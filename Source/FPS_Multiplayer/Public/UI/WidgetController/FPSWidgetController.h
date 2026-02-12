// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FPSWidgetController.generated.h"

/**
 * FUIWidgetControllerParams
 *
 * Struct for passing references to all systems a widget controller needs.
 * - BlueprintType: designers can initialize controllers from Blueprints.
 * - Members are TObjectPtr<> for GC awareness.
 */
USTRUCT(BlueprintType)
struct FUIWidgetControllerParams
{
	GENERATED_BODY()

	/** Default constructor: initializes pointers to nullptr (safe default). */
	FUIWidgetControllerParams() = default;

	/**
	 * Parameterized constructor for initializing all system references at once.
	 * @param InPlayerController         Owner player controller (HUD/input).
	 * @param InPlayerState              Persistent/replicated player state.
	 */
	explicit FUIWidgetControllerParams(APlayerController* InPlayerController,
	                                 APlayerState* InPlayerState
	                                 )
	    : PlayerController(InPlayerController)
	    , PlayerState(InPlayerState)
	{}

	/** Owner player controller; may be null until possession/creation completes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WidgetController")
	TObjectPtr<APlayerController> PlayerController = nullptr;

	/** Player state for identity/score/persistent data. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WidgetController")
	TObjectPtr<APlayerState> PlayerState = nullptr;
};

/**
 * 
 */
UCLASS()
class FPS_MULTIPLAYER_API UFPSWidgetController : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * Initializes all references at once from a single struct.
	 * Call immediately after constructing the controller and before any UI binding.
	 * Safe to call multiple times to refresh references (e.g., on pawn possession changes).
	 */
	UFUNCTION(BlueprintCallable, Category = "WidgetController")
	void SetWidgetControllerParams(const FUIWidgetControllerParams& InWidgetControllerParams);

	/**
	 * Broadcast the initial values of attributes to the UI.
	 * Derived classes should override and push current values (e.g., health/ammo)
	 * after widgets have bound to the controller's delegates.
	 */
	UFUNCTION(BlueprintCallable, Category = "WidgetController")
	virtual void BroadcastInitialValues();

	/**
	 * Bind status/effect change callbacks to the core systems.
	 * Derived classes should override and subscribe to their respective delegates
	 * to forward updates to widgets via controller delegates.
	 */
	virtual void BindCallbacksToDependencies();
	
protected:

	/** Owning player controller (HUD/input). */
	UPROPERTY(BlueprintReadOnly, Category = "Widget Controller")
	TObjectPtr<APlayerController> PlayerController;

	/** Owning player state (persistent/replicated player info). */
	UPROPERTY(BlueprintReadOnly, Category = "Widget Controller")
	TObjectPtr<APlayerState> PlayerState;
};
