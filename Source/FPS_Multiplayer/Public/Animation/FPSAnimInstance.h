// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "FPSAnimInstance.generated.h"

// Forward declarations to reduce compile time dependencies
class UCharacterMovementComponent;
class AFPSPlayerCharacter;

/**
 * Defines the character's facing/movement strategy.
 * Used to select the correct BlendSpace (Forward vs Backward) to prevent mesh distortion
 * when strafing at extreme angles (e.g., walking backwards while looking forwards).
 */
UENUM(BlueprintType)
enum class EMovementDirectionMode : uint8
{
	Forward  UMETA(DisplayName = "Forward"),
	Backward UMETA(DisplayName = "Backward")
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
	//                        ESSENTIAL MOVEMENT DATA
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
	
	// =========================================================================
	//                        LOCOMOTION CALCULATIONS
	// =========================================================================

	// The direction relative to the actor rotation (-180 to 180).
	// 0 = Forward, 90 = Right, -90 = Left, 180 = Backward.
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion | Direction")
	float Direction; 

	// Cached version of Direction (synced for now, can be smoothed later).
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion | Direction")
	float LocomotionDirection; 

	/**
	 * Represents the current movement style.
	 * 0.0 = Idle
	 * 1.0 = Walk
	 * 2.0 = Run
	 * 3.0 = Sprint
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion | Gait")
	float GaitValue;

	/**
	 * Stride Warping Multiplier.
	 * 1.0 = Normal Speed.
	 * >1.0 = Faster animation (feet move faster).
	 * <1.0 = Slower animation (feet move slower).
	 * Used to match animation foot speed to actual capsule movement speed.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion | Stride Warping")
	float PlayRate = 1.0f;
	
	// --- MOVEMENT DIRECTION SWITCHING ---
    
	// Decides if we use the Forward or Backward BlendSpace.
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion | Direction")
	EMovementDirectionMode MovementDirectionMode = EMovementDirectionMode::Forward;

	// --- ROTATION HISTORY (For Future Inertia/Stops) ---
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion | History")
	FRotator LastVelocityRotation;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion | History")
	bool bHasMovementInput;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion | History")
	FRotator LastMovementInputRotation;
	
	// Difference between Input Rotation and Velocity Rotation
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion | History")
	float MovementInputVelocityDifference;

	// =========================================================================
	//                           CONFIGURATION
	// =========================================================================

	// --- DIRECTION THRESHOLDS ---
	// Buffer determines how much "stickiness" the direction switch has to prevent flickering.
	UPROPERTY(EditDefaultsOnly, Category = "Configuration | Thresholds")
	float Buffer = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Configuration | Thresholds")
	float DirectionThresholdMax = 90.f;

	UPROPERTY(EditDefaultsOnly, Category = "Configuration | Thresholds")
	float DirectionThresholdMin = -90.0f;

	// --- SPEED CONFIGURATION ---
	// Defines the speed at which the character is considered "Walking" (Gait 1.0)
	UPROPERTY(EditDefaultsOnly, Category = "Configuration | Speeds")
	float WalkSpeed = 150.f;
	float WalkGaitValue = 1.f;

	// Defines the speed at which the character is considered "Running" (Gait 2.0)
	UPROPERTY(EditDefaultsOnly, Category = "Configuration | Speeds")
	float RunSpeed = 300.f;
	float RunGaitValue = 2.f;
	
	// Defines the speed at which the character is considered "Sprinting" (Gait 3.0)
	UPROPERTY(EditDefaultsOnly, Category = "Configuration | Speeds")
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
};