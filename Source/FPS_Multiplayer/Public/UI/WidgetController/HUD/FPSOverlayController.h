// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/FPSWidgetController.h"
#include "Data/Enums/FPSCharacterTypes.h"
#include "FPSOverlayController.generated.h"

class AFPSWeapon;
class UFPSCombatComponent;

// --- DELEGATES ---
// The View (Widget) binds to these. The Controller broadcasts to them.

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponAmmoChangedUISignature, int32, CurrentClip, int32, MaxClip);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCarriedAmmoChangedUISignature, int32, CarriedAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponIconChangedUISignature, UTexture2D*, Icon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrosshairChangedUISignature, UTexture2D*, Crosshair);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGaitChangedUISignature, EGait, NewGait);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOverlayStateChangedUISignature, EOverlayState, NewOverlayState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAimStateChangedUISignature, EAimState, NewAimState);


/**
 * Controller specifically for the Main HUD Overlay (Health, Ammo, Score).
 */
UCLASS(BlueprintType, Blueprintable)
class FPS_MULTIPLAYER_API UFPSOverlayController : public UFPSWidgetController
{
	GENERATED_BODY()
	
public:
	
	virtual void BeginDestroy() override;
	
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;
    
	// We broadcast Current AND Max ammo together for easier UI updates (e.g., "30 / 120")
	UPROPERTY(BlueprintAssignable, Category="Combat|Ammo")
	FOnWeaponAmmoChangedUISignature OnWeaponAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category="Combat|Ammo")
	FOnCarriedAmmoChangedUISignature OnCarriedAmmoChanged;
	
	UPROPERTY(BlueprintAssignable, Category="Combat|UI")
	FOnWeaponIconChangedUISignature OnWeaponIconChanged;
	
	UPROPERTY(BlueprintAssignable, Category="Combat|UI")
	FOnCrosshairChangedUISignature OnCrosshairChangedUI;
	
	UPROPERTY(BlueprintAssignable, Category="Player|State")
	FOnGaitChangedUISignature OnGaitChangedUI;
		
	UPROPERTY(BlueprintAssignable, Category="Player|State")
	FOnOverlayStateChangedUISignature OnOverlayStateChangedUI;
	
	UPROPERTY(BlueprintAssignable, Category="Player|State")
	FOnAimStateChangedUISignature OnAimStateChangedUI;
	
protected:
	
	UFUNCTION(BlueprintCallable)
	void OnWeaponEquipped(AFPSWeapon* NewWeapon);
	
	// Helper to find the Combat Component easily
	UFPSCombatComponent* GetCombatComponent() const;
};
