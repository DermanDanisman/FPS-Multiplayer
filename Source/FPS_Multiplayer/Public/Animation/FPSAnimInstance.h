// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Data/Structs/FPSCharacterDataContainer.h"
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

UENUM(BlueprintType)
enum class ECardinalDirection : uint8
{
	ECD_Forward     UMETA(DisplayName = "Forward"),
	ECD_Backward    UMETA(DisplayName = "Backward"),
	ECD_Left        UMETA(DisplayName = "Left"),
	ECD_Right       UMETA(DisplayName = "Right")
};

UENUM(BlueprintType)
enum class ELocomotionStartDirection : uint8
{
	LSD_Forward			UMETA(DisplayName = "Forward"),
	LSD_Right			UMETA(DisplayName = "Right"),
	LSD_Left			UMETA(DisplayName = "Left"),
	LSD_BackwardRight   UMETA(DisplayName = "Backward Right"),
	LSD_BackwardLeft	UMETA(DisplayName = "Backward Left"),
};

UENUM(BlueprintType)
enum class ELocomotionState : uint8
{
	ELS_Idle		UMETA(DisplayName = "Idle"),
	ELS_Pivoting	UMETA(DisplayName = "Pivoting"),
	ELS_Walk		UMETA(DisplayName = "Walk"),
	ELS_Run			UMETA(DisplayName = "Run"),
	ELS_Sprint		UMETA(DisplayName = "Sprint")
};

/**
 * Main Animation Instance for the First Person Character.
 * * ARCHITECTURE:
 * This class uses a "Gather-Read" architecture for thread safety.
 * 1. NativeUpdateAnimation (Game Thread): Extracts data from the Character and calculates variables.
 * 2. AnimGraph (Worker Thread): Reads these variables to drive animations in parallel.
 */
UCLASS(PrioritizeCategories="PlayerAnimInstance")
class FPS_MULTIPLAYER_API UFPSAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	// --- LIFECYCLE ---
	
	// Called once when the game starts (similar to BeginPlay)
	virtual void NativeInitializeAnimation() override;
	
	virtual void NativeUninitializeAnimation() override;

	// Called every frame. This is the main update loop.
	// Runs on Game Thread -> Safe to access Character & MovementComponent.
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
	virtual void NativePostEvaluateAnimation() override;

protected:
	
	// =========================================================================
	//                           REFERENCES
	// =========================================================================
	
	// Weak Pointers prevent crashes if the character is destroyed while the AnimInstance is still updating.
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|References")
	TWeakObjectPtr<AFPSPlayerCharacter> FPSCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|References")
	TWeakObjectPtr<UCharacterMovementComponent> MovementComponent;
	
	// =========================================================================
	//                        ANIMATION STATE DATA
	// =========================================================================
	
	// Current velocity vector (World Space)
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Essential Data")
	FVector Velocity;
	
	// Current velocity vector (World Space)
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Essential Data")
	FVector Velocity2D;
	
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Essential Data")
	FVector Acceleration;
	
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Essential Data")
	FVector InputVector;

	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Essential Data")
	float Speed3D;
	
	// Speed across the ground (ignoring Z/Vertical movement)
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Essential Data")
	float GroundSpeed;

	// True if the player is inputting movement AND has speed > Threshold
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Essential Data")
	bool bShouldMove; 

	// True if the character is in the air
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Essential Data")
	bool bIsFalling;
	
	// True if the character is crouching
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Essential Data")
	bool bIsCrouching;
	
	// True if the character is able to jump
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Essential Data")
	bool bCanJump;

	// True if input acceleration is non-zero (pressing WASD)
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Essential Data")
	bool bIsAccelerating;
	
	// [NEW] Helper for AnimGraph Transitions: Are we in the Sprinting State?
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Essential Data")
	bool bIsSprinting;

	// [NEW] Helper for AnimGraph Transitions: Are we Aiming Down Sights?
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Essential Data")
	bool bIsAiming;
	
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Essential Data")
	bool bIsArmed;
	
	// The "Source of Truth" state struct replicated from the Character
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Essential Data")
	FCharacterLayerStates LayerStates;
	
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Essential Data")
	bool bIsLocallyControlled;
	
	// =========================================================================
	//                        LOCOMOTION MATH
	// =========================================================================

	// Direction (-180 to 180) relative to Actor Rotation
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Locomotion")
	float Direction = 0.f; 
	
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Locomotion")
	ELocomotionState LocomotionState = ELocomotionState::ELS_Idle;
	
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Locomotion")
	ECardinalDirection CardinalDirection = ECardinalDirection::ECD_Forward;
	
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Locomotion")
	float OrientationWarpingAngle;

	// Smoothed/Cached direction
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Locomotion")
	float LocomotionDirection; 

	// 0=Idle, 1=Walk, 2=Run, 3=Sprint (Driven by curve mapping)
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Locomotion")
	float GaitValue;

	// Stride Warping: Multiplier for play speed to match capsule speed (Prevents foot sliding)
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Locomotion")
	float PlayRate = 1.0f;
    
	// Forward vs Backward selection
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Locomotion")
	EMovementDirectionMode MovementDirectionMode = EMovementDirectionMode::EMDM_Forward;

	// --- STOP PREDICTION ---
	
	// Used to play specific "Plant and Stop" animations based on which way we were moving
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Locomotion|History")
	FRotator LastVelocityRotation;
    
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Locomotion|History")
	bool bHasMovementInput;
    
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Locomotion|History")
	FRotator LastMovementInputRotation;
    
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Locomotion|History")
	float MovementInputVelocityDifference;
	
	// =========================================================================
	//                   --- PROCEDURAL MATH SETTINGS (Logic) ---
	// =========================================================================
	
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Turn In Place")
	float RootYawOffset;
	FRotator MovingRotation;
	FRotator LastMovingRotation;
	float DistanceCurve;
	float LastDistanceCurve;
	float DeltaDistanceCurve;
	float AbsoluteRootYawOffset;
	float YawExcess;
	FRotator LookingRotation;
	float YawRate;
	float LastRootYawOffset;
	bool bIsFirstTurnUpdate = true;
	
	UPROPERTY(EditDefaultsOnly, Category = "PlayerAnimInstance|AimOffset")
	float TurnAngle = 85.f;
	
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|AimOffset")
	float YawOffset;
	
	// Vertical Aim Offset (Pitch) used for Spine bending
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|AimOffset")
	float PitchOffset;
    
	// Distributed pitch value (e.g. Total Pitch / 8 bones) for smooth spine curvature
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|AimOffset")
	FRotator PitchValuePerBone;
	
	/**
	 * Controls the interpolation speed for the Procedural Hand Transform (TInterpTo).
	 * @usage Determines how "snappy" the sight alignment feels. 
	 * @value Higher (20+) = Snappy/Instant alignment. 
	 * @value Lower (10-) = Smooth/Floaty alignment (gun feels heavy).
	 * @note This is INDEPENDENT of the animation speed. You generally want this HIGH so the crosshair aligns quickly even if the animation is slow.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "PlayerAnimInstance|AimOffset")
	float AimInterpSpeed = 15.0f;
    
	// The Cache: Stores the calculated transform for EVERY sight on the gun.
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Aiming|Positioning")
	TArray<FTransform> HandTransforms;
	
	// The actual transform applied to the bone this frame (after interpolation)
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Aiming|Positioning")
	FTransform CurrentHandTransform;
    
	// The Active Transform: The specific one we are using RIGHT NOW (driven by CurrentSightIndex)
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Aiming|Positioning")
	FVector HandLocation;

	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Aiming|Positioning")
	FRotator HandRotation;
	
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Aiming|FABRIK")
	FTransform LeftHandTargetTransform;
	
	// Transition speeds loaded from Weapon Config
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Aiming|Positioning")
	float TimeToAim;
    
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Aiming|Positioning")
	float TimeFromAim;
	
	// CACHED VARIABLES (Updated only when weapon changes)
	
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Aiming|Positioning")
	FTransform CurrentHipFireOffset;

	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Aiming|Positioning")
	float CurrentDistanceFromCamera;
	
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Aiming|Two Bone IK")
	FVector CurrentRightHandEffectorLocation;
	
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Aiming|Two Bone IK")
	FVector CurrentRightHandJointTargetLocation;
	
	// Which sight slot are we using? (0 = Iron Sights, 1 = Red Dot, etc.)
	UPROPERTY(BlueprintReadOnly, Category = "PlayerAnimInstance|Aiming|State")
	int32 CurrentSightIndex = 0;
    
	// Add this struct inside your Class or above it
	struct FCachedSightData
	{
		// Is this an Optic or Iron Sight?
		bool bIsOptic; 
    
		// The Index (0, 1, 2)
		int32 SightIndex; 
    
		// Pointer to the specific mesh (so we don't iterate components)
		TWeakObjectPtr<UMeshComponent> Mesh; 
    
		// The Socket Names we found (so we don't check tags)
		// Primary Socket (Optic or Front Post)
		FName SocketNameA;
		// Secondary Socket (Rear Notch) - Used for Iron Sights
		FName SocketNameB;
    
		// [NEW] If the rear sight is on a DIFFERENT component (Modular), store it here.
		// If this is nullptr, we assume SocketNameB is on the MAIN Mesh.
		TWeakObjectPtr<UMeshComponent> RearMesh;
		
		// Helper to sort array by Index
		bool operator<(const FCachedSightData& Other) const
		{
			return SightIndex < Other.SightIndex;
		}
	};

	// The clean list of sights. No strings, just data.
	TArray<FCachedSightData> CachedSights;
	
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
	UPROPERTY(EditDefaultsOnly, Category = "PlayerAnimInstance|Configuration|Thresholds")
	float Buffer = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "PlayerAnimInstance|Configuration|Thresholds")
	float DirectionThresholdMax = 90.f;

	UPROPERTY(EditDefaultsOnly, Category = "PlayerAnimInstance|Configuration|Thresholds")
	float DirectionThresholdMin = -90.0f;
	
	// --- AIM CONFIGURATIONS ---
	
	// Higher = Snappier. Lower = Smoother (but "laggier"). 
	// Start with 15.0f.
	UPROPERTY(EditDefaultsOnly, Category = "PlayerAnimInstance|Configuration|AimOffset")
	float PitchPerBoneInterpSpeed = 10.0f;
	
	// --- SKELETON CONFIGURATION ---
	
	UPROPERTY(EditDefaultsOnly, Category = "PlayerAnimInstance|Configuration|Skeleton")
	FName HandBoneName = FName("hand_r");

	UPROPERTY(EditDefaultsOnly, Category = "PlayerAnimInstance|Configuration|Skeleton")
	FName HeadBoneName = FName("head");

	// --- SPEED CONFIGURATION ---
	
	// Defines the speed at which the character is considered "Walking" (Gait 1.0)
	UPROPERTY(EditDefaultsOnly, Category = "PlayerAnimInstance|Configuration|Dynamic Speeds")
	float WalkSpeed = 200.f;
	float WalkGaitValue = 1.f;

	// Defines the speed at which the character is considered "Running" (Gait 2.0)
	UPROPERTY(EditDefaultsOnly, Category = "PlayerAnimInstance|Configuration|Dynamic Speeds")
	float RunSpeed = 400.f;
	float RunGaitValue = 2.f;
	
	// Defines the speed at which the character is considered "Sprinting" (Gait 3.0)
	UPROPERTY(EditDefaultsOnly, Category = "PlayerAnimInstance|Configuration|Dynamic Speeds")
	float SprintSpeed = 600.f;
	float SprintGaitValue = 3.f;
	
private:
	// --- INTERNAL FUNCTIONS ---
	
	void UpdateReferences();
	void CalculateEssentialData();
	void CalculateLocomotionDirection();
	void CalculateCardinalDirection();
	void CalculateOrientationWarpingAngle();
	void CalculateMovementDirectionMode();
	void CalculateGaitValue();
	void CalculatePlayRate();
	void CalculatePitchValuePerBone();
	void CalculateAimOffset();
	void CalculateTurnInPlace();
	void CalculateLeftHandTransform();
	
	/**
	 * Calculates the precise hand offsets needed to align the weapon sights with the camera.
	 * This is an EVENT-BASED calculation (runs only when weapon changes), not per-frame.
	 */
	void CalculateHandTransforms();

	/**
	 * New Locomotion System Functions
	 */
	void CalculateLocomotionState();
	
	void CalculateCharacterRotation();
	
	void CalculateRotationWhileMoving();
	
	void CalculateLocomotionStartDirection(float StartAngle);
	
	/**
	 * Delegates
	 */
	// Event listener: Fires when the Combat Component successfully equips a new gun
	UFUNCTION()
	void OnCharacterWeaponEquipped(AFPSWeapon* NewWeapon);
	
	FDelegateHandle OnWeaponEquippedDelegateHandle;
	
protected:
	
	UPROPERTY(BlueprintReadOnly)
	FRotator StartRotation;
	
	UPROPERTY(BlueprintReadOnly)
	FRotator PrimaryRotation;
	
	UPROPERTY(BlueprintReadOnly)
	FRotator SecondaryRotation;
	
	UPROPERTY(BlueprintReadOnly)
	float LocomotionStartAngle;
	
	UPROPERTY(BlueprintReadOnly)
	ELocomotionStartDirection LocomotionStartDirection;
	
	UPROPERTY(BlueprintReadOnly)
	float LocomotionPlayRate;
		
	UFUNCTION(BlueprintCallable)
	void UpdateOnRunEnter();
	
	UFUNCTION(BlueprintCallable)
	void UpdateLocomotionPlayRate();
};