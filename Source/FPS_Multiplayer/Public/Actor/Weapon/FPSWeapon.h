// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Enums/CharacterTypes.h"
#include "GameFramework/Actor.h"
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
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement Limits")
	float MaxBaseSpeed = 600.f; // Standard "Run" speed
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement Limits")
	float MaxCrouchSpeed = 300.f;

	// --- ANIMATION REFERENCES (Stride Warping Math) ---
	// How fast the character moves in the ANIMATION file.
	
	UPROPERTY(EditDefaultsOnly, Category = "Animation Standards")
	float AnimWalkRefSpeed = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation Standards")
	float AnimRunRefSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation Standards")
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

UCLASS()
class FPS_MULTIPLAYER_API AFPSWeapon : public AActor
{
	GENERATED_BODY()

public:
	
	AFPSWeapon();
	
	// Getters for private components (Best practice for encapsulation)
	UFUNCTION(BlueprintPure, Category = "Getters")
	FORCEINLINE UWidgetComponent* GetInteractionWidget() const { return InteractionWidget; }
	
	UFUNCTION(BlueprintPure, Category = "Getters")
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	
	UFUNCTION(BlueprintCallable, Category = "Setters")
	void SetWeaponState(const EWeaponState NewWeaponState);
	
	UFUNCTION(BlueprintPure, Category = "Getters")
	FORCEINLINE EWeaponState GetWeaponState() const { return WeaponState; }
	
	// Getter for the Combat Component to read
	UFUNCTION(BlueprintPure, Category = "Getters")
	FORCEINLINE FWeaponMovementData GetMovementData() const { return MovementData; }
	
	UFUNCTION(BlueprintPure, Category = "Getters")
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() { return WeaponMesh; }

protected:
	
	virtual void BeginPlay() override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	/* * Overlap functions bound to the AreaSphere.
	 * NOTE: These functions will ONLY execute on the Server.
	 * Why? Because in BeginPlay, we bind them inside a 'HasAuthority()' check.
	 * This prevents Clients from spoofing overlaps or cheating.
	 */ 
	UFUNCTION()
	virtual void OnAreaSphereOverlap(
		UPrimitiveComponent* OverlappedComponent, 
		AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex, 
		bool bFromSweep, 
		const FHitResult& SweepResult
		);
	
	UFUNCTION()
	virtual void OnAreaSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent, 
		AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex
		);
	
	UFUNCTION()
	virtual void OnRep_WeaponState();

private:
	
	/* * Main visual representation of the gun. 
	 * Uses SkeletalMesh instead of StaticMesh because weapons need animations (reloading, bolt action).
	 */
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;
	
	/* * The "Trigger Zone" for picking up the weapon.
	 * In Multiplayer, we keep this disabled on Clients and Enabled on Server.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<USphereComponent> AreaSphere;
	
	/*
	 * The 3D UI that appears above the gun ("Press E to Pickup").
	 * By default, visibility is FALSE.
	 * The Character class controls this visibility via Replication, not the Weapon itself.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	TObjectPtr<UWidgetComponent> InteractionWidget;
	
	/*
	 * Tracks the current state (Initial, Equipped, Dropped).
	 * VisibleAnywhere allows us to debug the state in the Editor details panel.
	 */
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_WeaponState, Category = "Weapon Properties")
	EWeaponState WeaponState;
	
	// The configuration for THIS specific weapon
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Config")
	FWeaponMovementData MovementData;
};