// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Structs/CharacterDataContainer.h"
#include "FPSAnimInstance.generated.h"

// Forward declarations to reduce compile time dependencies
class UCharacterMovementComponent;
class AFPSPlayerCharacter;
class AFPSWeapon;

/**
 * Defines the character's facing/movement strategy.
 * Used to select the correct BlendSpace (Forward vs Backward) to prevent mesh distortion
 * when strafing at extreme angles (e.g., walking backwards while looking forwards).
 */
UENUM(BlueprintType)
enum class EMovementDirectionMode : uint8
{
	EMDM_Forward  UMETA(DisplayName = "Forward"),
	EMDM_Backward UMETA(DisplayName = "Backward")
};

/**
 * Main Animation Instance for the First Person Character.
 * * ARCHITECTURE:
 * This class uses a "Gather-Read" architecture for thread safety.
 * 1. NativeUpdateAnimation (Game Thread): Extracts data from the Character and calculates variables.
 * 2. AnimGraph (Worker Thread): Reads these variables to drive animations in parallel.
 */
UCLASS()
class FPS_MULTIPLAYER_API UFPSAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	// --- LIFECYCLE ---
	
	// Called once when the game starts (similar to BeginPlay)
	virtual void NativeInitializeAnimation() override;
	
	// Called every frame. This is the main update loop.
	// Runs on Game Thread -> Safe to access Character & MovementComponent.
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
	/**
	 * Calculates the precise hand offsets needed to align the weapon sights with the camera.
	 * This is an EVENT-BASED calculation (runs only when weapon changes), not per-frame.
	 */
	void CalculateHandTransforms();

protected:
	
	// =========================================================================
	//                           REFERENCES
	// =========================================================================
	
	// Weak Pointers prevent crashes if the character is destroyed while the AnimInstance is still updating.
	UPROPERTY(BlueprintReadOnly, Category = "References")
	TWeakObjectPtr<AFPSPlayerCharacter> FPSCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "References")
	TWeakObjectPtr<UCharacterMovementComponent> MovementComponent;
	
	// =========================================================================
	//                        ANIMATION STATE DATA
	// =========================================================================
	
	// Current velocity vector (World Space)
	UPROPERTY(BlueprintReadOnly, Category = "Essential Data")
	FVector Velocity;

	// Speed across the ground (ignoring Z/Vertical movement)
	UPROPERTY(BlueprintReadOnly, Category = "Essential Data")
	float GroundSpeed;

	// True if the player is inputting movement AND has speed > Threshold
	UPROPERTY(BlueprintReadOnly, Category = "Essential Data")
	bool bShouldMove; 

	// True if the character is in the air
	UPROPERTY(BlueprintReadOnly, Category = "Essential Data")
	bool bIsFalling;
	
	// True if the character is crouching
	UPROPERTY(BlueprintReadOnly, Category = "Essential Data")
	bool bIsCrouching;
	
	// True if the character is able to jump
	UPROPERTY(BlueprintReadOnly, Category = "Essential Data")
	bool bCanJump;

	// True if input acceleration is non-zero (pressing WASD)
	UPROPERTY(BlueprintReadOnly, Category = "Essential Data")
	bool bIsAccelerating;
	
	// The "Source of Truth" state struct replicated from the Character
	UPROPERTY(BlueprintReadOnly, Category = "Essential Data")
	FCharacterLayerStates LayerStates;
	
	// =========================================================================
	//                        LOCOMOTION MATH
	// =========================================================================

	// Direction (-180 to 180) relative to Actor Rotation
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Direction = 0.f; 

	// Smoothed/Cached direction
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float LocomotionDirection; 

	// 0=Idle, 1=Walk, 2=Run, 3=Sprint (Driven by curve mapping)
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float GaitValue;

	// Stride Warping: Multiplier for play speed to match capsule speed (Prevents foot sliding)
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float PlayRate = 1.0f;
    
	// Forward vs Backward selection
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	EMovementDirectionMode MovementDirectionMode = EMovementDirectionMode::EMDM_Forward;

	// --- STOP PREDICTION ---
	// Used to play specific "Plant and Stop" animations based on which way we were moving
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion | History")
	FRotator LastVelocityRotation;
    
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion | History")
	bool bHasMovementInput;
    
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion | History")
	FRotator LastMovementInputRotation;
    
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion | History")
	float MovementInputVelocityDifference;
	
	// =========================================================================
	//                        PROCEDURAL AIMING (FABRIK / IK)
	// =========================================================================
	
	// Vertical Aim Offset (Pitch) used for Spine bending
	UPROPERTY(BlueprintReadOnly, Category = "AimOffset")
	float PitchOffset;
    
	// Distributed pitch value (e.g. Total Pitch / 8 bones) for smooth spine curvature
	UPROPERTY(BlueprintReadOnly, Category = "AimOffset")
	FRotator PitchValuePerBone;
    
	// The Cache: Stores the calculated transform for EVERY sight on the gun.
	UPROPERTY(BlueprintReadOnly, Category = "Sight Alignment")
	TArray<FTransform> HandTransforms;
    
	// The Active Transform: The specific one we are using RIGHT NOW (driven by CurrentSightIndex)
	UPROPERTY(BlueprintReadOnly, Category = "Aiming | Final")
	FVector HandLocation;

	UPROPERTY(BlueprintReadOnly, Category = "Aiming | Final")
	FRotator HandRotation;
    
	// Which sight slot are we using? (0 = Iron Sights, 1 = Red Dot, etc.)
	UPROPERTY(BlueprintReadOnly, Category = "Aiming | State")
	int32 CurrentSightIndex = 0;
    
	// Transition speeds loaded from Weapon Config
	UPROPERTY(EditDefaultsOnly, Category = "Aiming | Final")
	float TimeToAim;
    
	UPROPERTY(EditDefaultsOnly, Category = "Aiming | Final")
	float TimeFromAim;
    
	// Helper struct for sorting/debugging sights
	struct FSightMeshData
	{
		UMeshComponent* Mesh;
		int32 SightIndex;
	};
    
	TArray<FSightMeshData> OpticSights;
	TArray<FSightMeshData> FrontSights;

	// =========================================================================
	//                           CONFIGURATION (Defaults)
	// =========================================================================

	// --- DIRECTION THRESHOLDS ---
	// Buffer determines how much "stickiness" the direction switch has to prevent flickering.
	UPROPERTY(EditDefaultsOnly, Category = "Configuration | Thresholds")
	float Buffer = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Configuration | Thresholds")
	float DirectionThresholdMax = 90.f;

	UPROPERTY(EditDefaultsOnly, Category = "Configuration | Thresholds")
	float DirectionThresholdMin = -90.0f;
	
	// --- AIM CONFIGURATIONS ---
	// Higher = Snappier. Lower = Smoother (but "laggier"). 
	// Start with 15.0f.
	UPROPERTY(EditDefaultsOnly, Category = "Configuration | AimOffset")
	float PitchPerBoneInterpSpeed = 15.0f;
	
	// --- SKELETON CONFIGURATION ---
	UPROPERTY(EditDefaultsOnly, Category = "Configuration | Skeleton")
	FName HandBoneName = FName("hand_r");

	UPROPERTY(EditDefaultsOnly, Category = "Configuration | Skeleton")
	FName HeadBoneName = FName("head");

	// --- SPEED CONFIGURATION ---
	// Defines the speed at which the character is considered "Walking" (Gait 1.0)
	UPROPERTY(EditDefaultsOnly, Category = "Configuration | Dynamic Speeds")
	float WalkSpeed = 150.f;
	float WalkGaitValue = 1.f;

	// Defines the speed at which the character is considered "Running" (Gait 2.0)
	UPROPERTY(EditDefaultsOnly, Category = "Configuration | Dynamic Speeds")
	float RunSpeed = 300.f;
	float RunGaitValue = 2.f;
	
	// Defines the speed at which the character is considered "Sprinting" (Gait 3.0)
	UPROPERTY(EditDefaultsOnly, Category = "Configuration | Dynamic Speeds")
	float SprintSpeed = 600.f;
	float SprintGaitValue = 3.f;
	
private:
	// --- INTERNAL FUNCTIONS ---
	void UpdateReferences();
	void CalculateEssentialData();
	void CalculateLocomotionDirection();
	void CalculateMovementDirectionMode();
	void CalculateGaitValue();
	void CalculatePlayRate();
	void CalculatePitchValuePerBone();
	void CalculateAimOffset();
	
	// Event listener: Fires when the Combat Component successfully equips a new gun
	UFUNCTION()
	void OnCharacterWeaponEquipped(AFPSWeapon* NewWeapon);
};