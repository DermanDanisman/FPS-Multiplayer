// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/DataAsset/FPSWeaponData.h"
#include "GameFramework/Actor.h"
#include "Interface/FPSInteractableInterface.h"
#include "FPSWeapon.generated.h"

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
	// 0: State when the weapon is spawned in the world waiting for a player.
	EWS_Initial UMETA(DisplayName = "Idle"), 
	
	// 1: State when the weapon is currently held by a Character.
	// Physics and Collision should be disabled here so it doesn't freak out while attached to the mesh.
	EWS_Equipped UMETA(DisplayName = "Equipped"), 
	
	// 2: State when the player throws the weapon.
	// We will re-enable physics here so it bounces off walls/floors.
	EWS_Dropped UMETA(DisplayName = "Dropped"), 
	
	EWS_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS(PrioritizeCategories = "Weapon")
class FPS_MULTIPLAYER_API AFPSWeapon : public AActor, public IFPSInteractableInterface
{
	GENERATED_BODY()

public:
	
	AFPSWeapon();
	
	// =========================================================================
	//                        GETTER FUNCTIONS
	// =========================================================================
	
	UFUNCTION(BlueprintPure, Category = "Weapon|Getters")
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() { return WeaponMesh; }
	
	UFUNCTION(BlueprintPure, Category = "Weapon|Getters")
	FORCEINLINE UWidgetComponent* GetInteractionWidget() const { return InteractionWidget; }
	
	UFUNCTION(BlueprintPure, Category = "Weapon|Getters")
	FORCEINLINE EWeaponState GetWeaponState() const { return WeaponState; }
	
	UFUNCTION(BlueprintPure, Category="Weapon|Getters")
	FORCEINLINE UFPSWeaponData* GetWeaponData() const { return WeaponData; }
	
	UFUNCTION(BlueprintPure, Category = "Weapon|Getters")
	FORCEINLINE FWeaponMovementData GetMovementData() const { return WeaponData ? WeaponData->WeaponMovementData : FWeaponMovementData(); } // Return the data if valid, otherwise return a default-constructed struct
	
	FORCEINLINE FName GetWeaponHandSocketName() const { return WeaponData ? WeaponData->WeaponHandSocketName : FName(); }
	
	FORCEINLINE float GetFireDelay() const { return WeaponData ? WeaponData->FireDelay : 0.1f; }
	FORCEINLINE bool IsAutomatic() const { return WeaponData ? WeaponData->bIsAutomatic : false; }
	FORCEINLINE int32 GetMaxClipAmmo() const { return WeaponData ? WeaponData->MaxClipAmmo : 30; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return WeaponData ? WeaponData->ReloadMontage : nullptr; }
	FORCEINLINE int32 GetCurrentClipAmmo() const { return CurrentClipAmmo; }
	FORCEINLINE void SetInitialClipAmmo() { CurrentClipAmmo = GetMaxClipAmmo(); }
	
	FORCEINLINE FTransform GetHipFireOffset() const { return WeaponData ? WeaponData->HipFireOffset : FTransform().Identity; }
	FORCEINLINE FVector GetRightHandEffectorLocation() const { return WeaponData ? WeaponData->RightHandEffectorLocation : FVector::ZeroVector; }
	FORCEINLINE FVector GetRightHandJointTargetLocation() const { return WeaponData ? WeaponData->RightHandJointTargetLocation : FVector::ZeroVector; }
	FORCEINLINE FVector GetLeftHandEffectorLocation() const { return WeaponData ? WeaponData->LeftHandEffectorLocation : FVector::ZeroVector; }
	FORCEINLINE FVector GetLeftHandJointTargetLocation() const { return WeaponData ? WeaponData->LeftHandJointTargetLocation : FVector::ZeroVector; }

	FORCEINLINE float GetTimeToAim() const { return WeaponData ? WeaponData->TimeToAim : 0.5f; }
	FORCEINLINE float GetTimeFromAim() const { return WeaponData ? WeaponData->TimeFromAim : 0.2f; }
	FORCEINLINE float GetDistanceFromCamera() const { return WeaponData ? WeaponData->DistanceFromCamera : 0.f; }
	
	FORCEINLINE FName GetOpticSocketName() const { return WeaponData ? WeaponData->OpticSocketName : FName(); }
	FORCEINLINE FName GetFrontSightSocketName() const { return WeaponData ? WeaponData->FrontSightSocketName : FName(); }
	FORCEINLINE FName GetRearSightSocketName() const { return WeaponData ? WeaponData->RearSightSocketName : FName(); }
	FORCEINLINE FString GetOpticTagPrefix() const { return WeaponData ? WeaponData->OpticTagPrefix : FString(); }
	FORCEINLINE FString GetFrontSightTagPrefix() const { return WeaponData ? WeaponData->FrontSightTagPrefix : FString(); }
	FORCEINLINE FString GetRearSightTagPrefix() const { return WeaponData ? WeaponData->RearSightTagPrefix : FString(); }
	
	// =========================================================================
	//                        SETTER FUNCTIONS
	// =========================================================================
	
	UFUNCTION(BlueprintCallable, Category = "Weapon|Setters")
	void SetWeaponState(const EWeaponState NewWeaponState);
	
	// =========================================================================
	//                        IFPSInteractableInterface FUNCTIONS
	// =========================================================================
	
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;
	virtual void OnFocusGained_Implementation(APawn* InstigatorPawn) override;
	virtual void OnFocusLost_Implementation(APawn* InstigatorPawn) override;
	
	// Called by CombatComponent. Needs the HitTarget to know where the bullet goes!
	void Fire(const FVector& HitTarget);
	
	void AddAmmo(int32 AmountToAdd);

protected:
	
	virtual void BeginPlay() override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	virtual void OnRep_WeaponState();
	
	// --- RPCs ---
	UFUNCTION(Server, Reliable)
	void Server_Fire(const FVector& TraceHitTarget);

	UFUNCTION(NetMulticast, Unreliable) // Unreliable is faster for FX!
	void Multicast_Fire(const FVector& TraceHitTarget);

	// Helper to play sounds/particles locally
	void PlayFireEffects(const FVector& TraceHitTarget) const;

private:
	
	/* * Main visual representation of the gun. 
	 * Uses SkeletalMesh instead of StaticMesh because weapons need animations (reloading, bolt action).
	 */
	UPROPERTY(VisibleAnywhere, Category = "Weapon|Properties")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;
	
	/*
	 * The 3D UI that appears above the gun ("Press E to Pickup").
	 * By default, visibility is FALSE.
	 * The Character class controls this visibility via Replication, not the Weapon itself.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Weapon|Properties")
	TObjectPtr<UWidgetComponent> InteractionWidget;
	
	// Single source of truth for configuration
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Properties")
	TObjectPtr<UFPSWeaponData> WeaponData;
	
	/*
	 * Tracks the current state (Initial, Equipped, Dropped).
	 * VisibleAnywhere allows us to debug the state in the Editor details panel.
	 */
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_WeaponState, Category = "Weapon|State")
	EWeaponState WeaponState;
	
	// This MUST remain on the Actor because it changes when you shoot!
	UPROPERTY(Replicated, EditAnywhere, Category = "Weapon|State")
	int32 CurrentClipAmmo;
};