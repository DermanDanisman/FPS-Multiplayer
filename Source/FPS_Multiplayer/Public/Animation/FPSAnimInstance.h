// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Character/FPSPlayerCharacter.h"
#include "Component/FPSCharacterMovementComponent.h"
#include "Data/Structs/FPSCharacterDataContainer.h"
#include "FPSAnimInstance.generated.h"

// Forward declarations to reduce compile time dependencies
class UFPSCharacterMovementComponent;
class UCharacterMovementComponent;
class AFPSPlayerCharacter;
class AFPSWeapon;

#pragma region Enums

/** Determines which start animation to play when moving from Idle. */
UENUM(BlueprintType)
enum class ELocomotionStartDirection : uint8
{
    LSD_Forward          UMETA(DisplayName = "Forward"),
    LSD_Right            UMETA(DisplayName = "Right"),
    LSD_Left             UMETA(DisplayName = "Left"),
    LSD_BackwardRight    UMETA(DisplayName = "Backward Right"),
    LSD_BackwardLeft     UMETA(DisplayName = "Backward Left"),
};

UENUM(BlueprintType)
enum class ELocomotionState : uint8
{
    ELS_Idle      UMETA(DisplayName = "Idle"),
    ELS_Walk      UMETA(DisplayName = "Walk"),
    ELS_Run       UMETA(DisplayName = "Run"),
    ELS_Sprint    UMETA(DisplayName = "Sprint")
};

#pragma endregion Enums

/**
 * Main Animation Instance for the First Person Character.
 * * ARCHITECTURE:
 * This class uses a "Gather-Read" architecture for thread safety and optimization.
 * 1. NativeUpdateAnimation (Game Thread): Extracts data from the Character/Components and calculates 'Transient' variables.
 * 2. AnimGraph (Worker Thread): Reads these pre-calculated variables to drive animations in parallel.
 * * NETWORKING NOTE:
 * Logic splits often occur between IsLocallyControlled() (The owning player) and !IsLocallyControlled() (Other players).
 */
UCLASS()
class FPS_MULTIPLAYER_API UFPSAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
    
public:
    // =========================================================================
    //                           LIFECYCLE
    // =========================================================================
    
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUninitializeAnimation() override;

    // Called every frame on the Game Thread.
    // Safe to access UObjects (Character, MovementComponent, Weapons).
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    
    // --- Getters ---
    
    UFUNCTION(BlueprintPure, Category = "References")
    FORCEINLINE UFPSCharacterMovementComponent* GetMovementComponent() const
    {
       return TryGetPawnOwner()->GetMovementComponent() ? Cast<UFPSCharacterMovementComponent>(TryGetPawnOwner()->GetMovementComponent()) : nullptr;
    }
    
    UFUNCTION(BlueprintPure, Category = "References")
    FORCEINLINE AFPSPlayerCharacter* GetFPSCharacter() const
    {
       return TryGetPawnOwner() ? Cast<AFPSPlayerCharacter>(TryGetPawnOwner()) : nullptr;
    }

protected:
    
    // =========================================================================
    //                        ESSENTIAL DATA (Gathered)
    // =========================================================================
    
    /** Current velocity vector in World Space. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Data")
    FVector Velocity;
    
    /** Current velocity ignoring Z axis (useful for ground speed). */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Data")
    FVector Velocity2D;
    
    /** Current acceleration (Input intent) from the Movement Component. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Data")
    FVector Acceleration;
    
    /** * Raw Input Vector. 
     * NOTE: This is only valid for the Local Player. Replicates as Zero for others. 
     */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Data")
    FVector InputVector;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Data")
    float Speed3D;
    
    /** Speed across the ground (cm/s). Drives blendspaces. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Data")
    float GroundSpeed;

    /** Gate: True if player has input AND speed > Threshold. Prevents micro-movements. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Data")
    bool bShouldMove; 

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Data")
    bool bIsFalling;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Data")
    bool bIsCrouching;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Data")
    bool bCanJump;

    /** True if input acceleration is non-zero (pressing WASD). */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Data")
    bool bIsAccelerating;
    
    /** Helper for AnimGraph: Are we in the Sprinting State? */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Data")
    bool bIsSprinting;

    /** Helper for AnimGraph: Are we Aiming Down Sights? */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Data")
    bool bIsAiming;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Data")
    bool bIsArmed;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Data")
    bool bIsLocallyControlled;
    
    /** The "Source of Truth" replicated from the Character (Gait, Stance, etc.). */
    UPROPERTY(BlueprintReadOnly, Category = "Essential Data")
    FCharacterLayerStates LayerStates;
    
    // =========================================================================
    //                        LOCOMOTION MATH
    // =========================================================================

    /** Direction (-180 to 180) relative to Actor Rotation. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
    float Direction = 0.f; 
    
    UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
    ELocomotionState LocomotionState = ELocomotionState::ELS_Idle;
    
    // --- START / STOP LOGIC ---
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
    FRotator StartRotation;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
    FRotator PrimaryRotation;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
    FRotator SecondaryRotation;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
    float LocomotionStartAngle;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
    ELocomotionStartDirection LocomotionStartDirection;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
    float DisplacementSpeed;

    // 0.0 = Idle, 1.0 = Walk, 2.0 = Run
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
    float LocomotionStateValue;
    
    // --- Stop Prediction / Distance Matching Data ---
    
    UPROPERTY(BlueprintReadOnly, Category = "Locomotion|History")
    FRotator LastVelocityRotation;
    
    UPROPERTY(BlueprintReadOnly, Category = "Locomotion|History")
    bool bHasMovementInput;
    
    UPROPERTY(BlueprintReadOnly, Category = "Locomotion|History")
    FRotator LastMovementInputRotation;
    
    UPROPERTY(BlueprintReadOnly, Category = "Locomotion|History")
    float MovementInputVelocityDifference;
    
    // =========================================================================
    //                   TURN IN PLACE & ROTATION
    // =========================================================================
    
    /** Offset between the Root Bone and the Actor Rotation. */
    UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
    float RootYawOffset;
    
    UPROPERTY(EditDefaultsOnly, Category = "AimOffset")
    float TurnAngle = 85.f;
    
    UPROPERTY(BlueprintReadOnly, Category = "AimOffset")
    float YawOffset;
    
    // Internal Turn In Place Variables
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

    // =========================================================================
    //                        AIM OFFSETS (Spine)
    // =========================================================================
    
    /** Vertical Aim Offset (Pitch) used for generic AimOffsets. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "AimOffset")
    float PitchOffset;
    
    /** * Distributed pitch value (Total Pitch / Num Bones). 
     * Used to smooth spine curvature so the character doesn't fold in half.
     */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "AimOffset")
    FRotator PitchValuePerBone;

    // =========================================================================
    //                  PROCEDURAL AIMING (ADS & WEAPONS)
    // =========================================================================
    
    /**
     * Controls interpolation speed for Procedural Hand Transform.
     * Higher (20+) = Snappy/Instant alignment. 
     * Lower (10-) = Smooth/Floaty alignment (gun feels heavy).
     */
    UPROPERTY(Transient, EditDefaultsOnly, Category = "Aiming")
    float AimInterpSpeed = 15.0f;
    
    /** * The Cache: Stores the calculated transform for EVERY sight on the gun.
     * Index 0 = Iron Sights, Index 1 = Red Dot, etc.
     */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    TArray<FTransform> HandTransforms;
    
    /** The actual transform applied to the hand bone this frame (after interpolation). */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    FTransform CurrentHandTransform;
    
    /** OUTPUT: The location to drive the IK/Bone modification in AnimGraph. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    FVector HandLocation;

    /** OUTPUT: The rotation to drive the IK/Bone modification in AnimGraph. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    FRotator HandRotation;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|FABRIK")
    FTransform LeftHandTargetTransform;
    
    // --- Cached Weapon Data (Updated on Equip) ---
    
    UPROPERTY(BlueprintReadOnly, Category = "Aiming|Positioning")
    float TimeToAim;
    
    UPROPERTY(BlueprintReadOnly, Category = "Aiming|Positioning")
    float TimeFromAim;
    
    UPROPERTY(BlueprintReadOnly, Category = "Aiming|Positioning")
    FTransform CurrentHipFireOffset;

    UPROPERTY(BlueprintReadOnly, Category = "Aiming|Positioning")
    float CurrentDistanceFromCamera;
    
    UPROPERTY(BlueprintReadOnly, Category = "Aiming|Two Bone IK")
    FVector CurrentRightHandEffectorLocation;
    
    UPROPERTY(BlueprintReadOnly, Category = "Aiming|Two Bone IK")
    FVector CurrentRightHandJointTargetLocation;
    
    /** Which sight slot are we using? (0 = Iron Sights, 1 = Red Dot, etc.) */
    UPROPERTY(BlueprintReadOnly, Category = "Aiming|State")
    int32 CurrentSightIndex = 0;
    
    /**
     * Optimized struct to hold pre-parsed sight data.
     * Avoids string manipulation and component iteration during runtime.
     */
    struct FCachedSightData
    {
       bool bIsOptic; 
       int32 SightIndex; 
       TWeakObjectPtr<UMeshComponent> Mesh; 
       FName SocketNameA; // Front/Optic
       FName SocketNameB; // Rear (if Irons)
       TWeakObjectPtr<UMeshComponent> RearMesh; // If rear sight is separate
       
       bool operator<(const FCachedSightData& Other) const { return SightIndex < Other.SightIndex; }
    };

    /** The clean list of sights. */
    TArray<FCachedSightData> CachedSights;
    
    // =========================================================================
    //                           CONFIGURATION
    // =========================================================================
    
    UPROPERTY(EditDefaultsOnly, Category = "Configuration|Skeleton")
    FName HandBoneName = FName("hand_r");

    UPROPERTY(EditDefaultsOnly, Category = "Configuration|Skeleton")
    FName HeadBoneName = FName("head");
    
    /** Interpolation speed for spine bending. Higher = Snappier. */
    UPROPERTY(EditDefaultsOnly, Category = "Configuration|AimOffset")
    float PitchPerBoneInterpSpeed = 10.0f;
    
    /** 
     * Controls how fast the locomotion state value (0=Idle, 1=Walk, 2=Run) smooths out. 
     * Higher = Snappier, Lower = Smoother blending.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Configuration|Movement")
    float LocomotionCurveInterpSpeed = 10.f;
    
    UPROPERTY(BlueprintReadOnly, Category = "Configuration|Data")
    float GroundDistance = -1.0f;
    
private:
    // Internal location tracking for displacement calc
    FVector WorldLocation;
    float DisplacementSinceLastUpdate;
    bool bIsFirstUpdate = true;
    FDelegateHandle OnWeaponEquippedDelegateHandle;

    // =========================================================================
    //                        INTERNAL FUNCTIONS
    // =========================================================================

    void UpdateReferences();
    
    /** Gathers simple float/bool/vector data from the character. */
    void CalculateEssentialData();
    
    /** Determines if we are Idle, Walking, Running, etc. */
    void CalculateLocomotionState();
    
    void CalculateLocationData(float DeltaTime);
    
    /** Determines which Start Animation (Left, Right, Forward) to play. */
    void CalculateLocomotionStartDirection(float StartAngle);
    
    /** Calculates smooth spine bending. */
    void CalculatePitchValuePerBone();
    
    /** Calculates basic Aim Offset (Pitch). */
    void CalculateAimOffset();
    
    /** Calculates Root Yaw Offset for standing turn animations. */
    void CalculateTurnInPlace();
    
    /** Calculates Left Hand IK target based on weapon socket. */
    void CalculateLeftHandTransform();
    
    /**
     * Calculates the precise hand offsets needed to align the weapon sights with the camera.
     * Handles both Local Player (True Camera) and Remote Player (Proxy Camera) logic.
     */
    void CalculateHandTransforms();
    
    /** Event listener: Fires when the Combat Component successfully equips a new gun. */
    UFUNCTION()
    void OnCharacterWeaponEquipped(AFPSWeapon* NewWeapon);
    
    UFUNCTION(BlueprintCallable)
    void UpdateOnLocomotionEnter();
};