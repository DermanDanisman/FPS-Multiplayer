// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/DataAsset/FPSWeaponData.h"
#include "GameFramework/Actor.h"
#include "Interface/FPSInteractableInterface.h"
#include "FPSWeapon.generated.h"

// --- Forward Declarations ---
class UFPSRecoilComponent;
class UWidgetComponent;
class USphereComponent;

/**
 * @enum EWeaponState
 * @brief Defines the current physical state of the weapon.
 * Crucial for networking to ensure Clients and Server agree on what the gun is doing.
 */
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	/** 0: State when the weapon is spawned in the world waiting for a player. */
	EWS_Initial UMETA(DisplayName = "Idle"), 
	
	/**
	 * 1: State when the weapon is currently held by a Character.
	 * Physics and Collision should be disabled here so it doesn't freak out while attached to the mesh.
	 */
	EWS_Equipped UMETA(DisplayName = "Equipped"), 
	
	/** 
	 * 2: State when the player throws the weapon.
	 * We will re-enable physics here so it bounces off walls/floors.
	 */
	EWS_Dropped UMETA(DisplayName = "Dropped"), 
	
	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

// Delegate declaration (Standard Multicast)
DECLARE_DELEGATE_TwoParams(FOnWeaponStateChanged, int32 /*Current*/, int32 /*Max*/);

/**
 * @class AFPSWeapon
 * @brief The base actor class for all weapons.
 * * Handles visual representation (Mesh), Configuration (DataAsset), and execution of
 * firing logic (FX, Ammo consumption).
 */
UCLASS(PrioritizeCategories = "Weapon")
class FPS_MULTIPLAYER_API AFPSWeapon : public AActor, public IFPSInteractableInterface
{
	GENERATED_BODY()

public:
	
	AFPSWeapon();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	// =========================================================================
	//                        PUBLIC API (Combat)
	// =========================================================================
    
	/**
	 * Executes the firing logic.
	 * 1. Updates Ammo (Prediction)
	 * 2. Plays FX (Prediction)
	 * 3. Calls Server to authoritative fire.
	 * @param HitTarget - The world location the trace hit (calculated by Camera).
	 */
	void Fire(const FVector& HitTarget);
	
	/**
	 * Adds ammo to the current magazine. Safe to call, handles clamping.
	 * @note Should typically only be called on the Server.
	 */
	void AddAmmo(int32 AmountToAdd);
    
	/** Resets ammo to full capacity (e.g. on spawn). */
	void SetInitialClipAmmo();
	
	void ApplyRecoil(int32 ShotsFired) const;
	
	void ResetRecoil() const;
	
	UFUNCTION(BlueprintPure, Category = "Weapon|Getters")
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	
	UFUNCTION(BlueprintPure, Category = "Weapon|Getters")
	FORCEINLINE UWidgetComponent* GetInteractionWidget() const { return InteractionWidget; }
	
	// =========================================================================
	//                        STATE MANAGEMENT
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category = "Weapon|Setters")
	void SetWeaponState(const EWeaponState NewWeaponState);
    
	UFUNCTION(BlueprintPure, Category = "Weapon|Getters")
	FORCEINLINE EWeaponState GetWeaponState() const { return WeaponState; }
	
	// =========================================================================
    //                        CONFIGURATION GETTERS (Data Asset Wrapper)
    // =========================================================================
    
    UFUNCTION(BlueprintPure, Category = "Weapon|Getters")
    FORCEINLINE UFPSWeaponData* GetWeaponData() const { return WeaponData; }
	UFUNCTION(BlueprintPure, Category = "Weapon|Getters")
	FORCEINLINE FString GetWeaponName() const { return WeaponData ? WeaponData->WeaponName : FString(); }
	UFUNCTION(BlueprintPure, Category = "Weapon|Getters")
	FORCEINLINE UTexture2D* GetWeaponIcon() const { return WeaponData ? WeaponData->WeaponIcon : nullptr; }
	UFUNCTION(BlueprintPure, Category = "Weapon|Getters")
	FORCEINLINE UTexture2D* GetCrosshairTexture() const { return WeaponData ? WeaponData->CrosshairTexture : nullptr; }
    
    // --- Shortcuts to DataAsset properties for cleaner code elsewhere ---
    FORCEINLINE float GetFireDelay() const { return WeaponData ? WeaponData->FireDelay : 0.1f; }
    FORCEINLINE bool IsAutomatic() const { return WeaponData ? WeaponData->bIsAutomatic : false; }
    FORCEINLINE int32 GetMaxClipAmmo() const { return WeaponData ? WeaponData->MaxClipAmmo : 30; }
    FORCEINLINE int32 GetCurrentClipAmmo() const { return CurrentClipAmmo; }
	FORCEINLINE float GetWeaponDamage() const { return WeaponData ? WeaponData->Damage : 100.f; }
    FORCEINLINE float GetWeaponRange() const { return WeaponData ? WeaponData->Range : 10000.f; }
    
    // --- Visuals & IK ---
    FORCEINLINE FName GetWeaponHandSocketName() const { return WeaponData ? WeaponData->WeaponHandSocketName : FName(); }
    FORCEINLINE FWeaponMovementData GetMovementData() const { return WeaponData ? WeaponData->WeaponMovementData : FWeaponMovementData(); }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return WeaponData ? WeaponData->ReloadMontage : nullptr; }
    
    // --- Procedural Aiming Getters ---
    FORCEINLINE FTransform GetHipFireOffset() const { return WeaponData ? WeaponData->HipFireOffset : FTransform().Identity; }
    FORCEINLINE float GetDistanceFromCamera() const { return WeaponData ? WeaponData->DistanceFromCamera : 0.f; }
    FORCEINLINE float GetTimeToAim() const { return WeaponData ? WeaponData->TimeToAim : 0.25f; }
    FORCEINLINE float GetTimeFromAim() const { return WeaponData ? WeaponData->TimeFromAim : 0.2f; }
    
    // --- IK Locators ---
    FORCEINLINE FVector GetRightHandEffectorLocation() const { return WeaponData ? WeaponData->RightHandEffectorLocation : FVector::ZeroVector; }
    FORCEINLINE FVector GetLeftHandEffectorLocation() const { return WeaponData ? WeaponData->LeftHandEffectorLocation : FVector::ZeroVector; }
    FORCEINLINE FVector GetRightHandJointTargetLocation() const { return WeaponData ? WeaponData->RightHandJointTargetLocation : FVector::ZeroVector; }
    FORCEINLINE FVector GetLeftHandJointTargetLocation() const { return WeaponData ? WeaponData->LeftHandJointTargetLocation : FVector::ZeroVector; }

    // --- Sight Config ---
    FORCEINLINE FName GetOpticSocketName() const { return WeaponData ? WeaponData->OpticSocketName : FName(); }
    FORCEINLINE FName GetFrontSightSocketName() const { return WeaponData ? WeaponData->FrontSightSocketName : FName(); }
    FORCEINLINE FName GetRearSightSocketName() const { return WeaponData ? WeaponData->RearSightSocketName : FName(); }
    FORCEINLINE FString GetOpticTagPrefix() const { return WeaponData ? WeaponData->OpticTagPrefix : FString(); }
    FORCEINLINE FString GetFrontSightTagPrefix() const { return WeaponData ? WeaponData->FrontSightTagPrefix : FString(); }
    FORCEINLINE FString GetRearSightTagPrefix() const { return WeaponData ? WeaponData->RearSightTagPrefix : FString(); }

    // =========================================================================
    //                        INTERFACE IMPLEMENTATION
    // =========================================================================
    virtual void Interact_Implementation(APawn* InstigatorPawn) override;
    virtual void OnFocusGained_Implementation(APawn* InstigatorPawn) override;
    virtual void OnFocusLost_Implementation(APawn* InstigatorPawn) override;

protected:
	
	virtual void BeginPlay() override;
    
	/** Called when WeaponState replicates to client. Updates physics/collision. */
	UFUNCTION()
	virtual void OnRep_WeaponState();
	
	// --- RPCs ---
    
	UFUNCTION(Server, Reliable)
	void Server_Fire(const FVector& TraceHitTarget);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_Fire(const FVector& TraceHitTarget);
	
	UFUNCTION()
	void OnRep_CurrentClipAmmo();

	/** 
	 * Local helper to play Muzzle Flash, Sound, and Impact Particles.
	 * @note Marked const as it spawns external actors/components, doesn't mutate Weapon state.
	 */
	void PlayFireEffects(const FVector& TraceHitTarget) const;

private:
	
	/**
	 * Main visual representation of the gun. 
	 * Uses SkeletalMesh instead of StaticMesh because weapons need animations (reloading, bolt action).
	 */
	UPROPERTY(VisibleAnywhere, Category = "Weapon|Properties")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;
	
	UPROPERTY(VisibleAnywhere, Category = "Weapon|Properties")
	TObjectPtr<UFPSRecoilComponent> RecoilComponent;
	
	/**
	 * The 3D UI that appears above the gun ("Press E to Pickup").
	 * By default, visibility is FALSE.
	 * The Character class controls this visibility via Replication, not the Weapon itself.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Weapon|Properties")
	TObjectPtr<UWidgetComponent> InteractionWidget;
	
	// Single source of truth for configuration
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Properties")
	TObjectPtr<UFPSWeaponData> WeaponData;
	
	/**
	 * Tracks the current state (Initial, Equipped, Dropped).
	 * VisibleAnywhere allows us to debug the state in the Editor details panel.
	 */
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_WeaponState, Category = "Weapon|State")
	EWeaponState WeaponState;
	
	// This MUST remain on the Actor because it changes when you shoot!
	UPROPERTY(ReplicatedUsing = OnRep_CurrentClipAmmo, EditAnywhere, Category = "Weapon|State")
	int32 CurrentClipAmmo;
	
public:
	
	// Bound to only CombatComponent
	FOnWeaponStateChanged OnAmmoChanged;
};