// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Enums/FPSCharacterTypes.h"
#include "GameFramework/Actor.h"
#include "Interface/FPSInteractableInterface.h"
#include "FPSWeapon.generated.h"

class UWidgetComponent;
class USphereComponent;

USTRUCT(BlueprintType)
struct FWeaponMovementData
{
	GENERATED_BODY()
	
	EOverlayState OverlayState = EOverlayState::EOS_Rifle;

	// --- PHYSICAL LIMITS (CharacterMovementComponent) ---
	// How fast the capsule is ALLOWED to move.
	
	UPROPERTY(EditAnywhere, Category = "Weapon|Movement Limits")
	float MaxBaseSpeed = 600.f; // Standard "Run" speed
	
	UPROPERTY(EditAnywhere, Category = "Weapon|Movement Limits")
	float MaxCrouchSpeed = 300.f;

	// --- ANIMATION REFERENCES (Stride Warping Math) ---
	// How fast the character moves in the ANIMATION file.
	
	UPROPERTY(EditAnywhere, Category = "Weapon|Animation Standards")
	float AnimWalkRefSpeed = 150.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Animation Standards")
	float AnimRunRefSpeed = 300.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Animation Standards")
	float AnimSprintRefSpeed = 600.f;
};

UENUM(BlueprintType)
enum EWeaponType
{
	// We will expand this with the child classes. Like for Melee: knife, sword etc. for Ranged: pistol, rifle etc.
	
	EWT_Melee  UMETA(DisplayName = "Melee"),
	EWR_Ranged UMETA(DisplayName = "Ranged")
};

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
	
	// Getters for private components (Best practice for encapsulation)
	UFUNCTION(BlueprintPure, Category = "Weapon|Getters")
	FORCEINLINE UWidgetComponent* GetInteractionWidget() const { return InteractionWidget; }
	
	UFUNCTION(BlueprintPure, Category = "Weapon|Getters")
	FORCEINLINE EWeaponState GetWeaponState() const { return WeaponState; }
	
	// Getter for the Combat Component to read
	UFUNCTION(BlueprintPure, Category = "Weapon|Getters")
	FORCEINLINE FWeaponMovementData GetMovementData() const { return MovementData; }
	
	UFUNCTION(BlueprintPure, Category = "Weapon|Getters")
	FORCEINLINE float GetDistanceFromCamera() const { return DistanceFromCamera; }
	
	// [NEW] Getter
	FORCEINLINE FTransform GetHipFireOffset() const { return HipFireOffset; }
	FORCEINLINE FVector GetLeftHandEffectorLocation() const { return LeftHandEffectorLocation; }
	FORCEINLINE float GetTimeToAim() const { return TimeToAim; }
	FORCEINLINE float GetTimeFromAim() const { return TimeFromAim; }
	FORCEINLINE FName GetOpticSocketName() const { return OpticSocketName; }
	FORCEINLINE FName GetFrontSightSocketName() const { return FrontSightSocketName; }
	FORCEINLINE FName GetRearSightSocketName() const { return RearSightSocketName; }
	FORCEINLINE FString GetOpticTagPrefix() const { return OpticTagPrefix; }
	FORCEINLINE FString GetFrontSightTagPrefix() const { return FrontSightTagPrefix; }
	FORCEINLINE FString GetRearSightTagPrefix() const { return RearSightTagPrefix; }
	
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

protected:
	
	virtual void BeginPlay() override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	virtual void OnRep_WeaponState();

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
	
	/*
	 * Tracks the current state (Initial, Equipped, Dropped).
	 * VisibleAnywhere allows us to debug the state in the Editor details panel.
	 */
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_WeaponState, Category = "Weapon|Properties")
	EWeaponState WeaponState;
	
	// The configuration for THIS specific weapon
	UPROPERTY(EditAnywhere, Category = "Weapon|Config")
	FWeaponMovementData MovementData;
	
	UPROPERTY(EditAnywhere, Category = "Weapon|Config|Aim")
	float DistanceFromCamera;
	
	UPROPERTY(EditAnywhere, Category = "Weapon|Config|Aim")
	float TimeToAim;
	
	UPROPERTY(EditAnywhere, Category = "Weapon|Config|Aim")
	float TimeFromAim;
	
	// [NEW] The target location for the weapon when NOT aiming (Hip Fire).
	// This is an offset relative to the Camera (Head).
	UPROPERTY(EditAnywhere, Category = "Weapon|Config|Positioning")
	FTransform HipFireOffset;
	
	UPROPERTY(EditAnywhere, Category = "Weapon|Config|Positioning")
	FVector LeftHandEffectorLocation;
	
	// --- SIGHT CONFIGURATION ---
    
	UPROPERTY(EditAnywhere, Category = "Weapon|Config|Sights")
	FName OpticSocketName = FName("OpticAimpoint");

	UPROPERTY(EditAnywhere, Category = "Weapon|Config|Sights")
	FName FrontSightSocketName = FName("FrontAimpoint");

	UPROPERTY(EditAnywhere, Category = "Weapon|Config|Sights")
	FName RearSightSocketName = FName("RearAimpoint");

	UPROPERTY(EditAnywhere, Category = "Weapon|Config|Sights")
	FString OpticTagPrefix = TEXT("OpticSight");

	UPROPERTY(EditAnywhere, Category = "Weapon|Config|Sights")
	FString FrontSightTagPrefix = TEXT("FrontSight");

	UPROPERTY(EditAnywhere, Category = "Weapon|Config|Sights")
	FString RearSightTagPrefix = TEXT("RearSight");
};