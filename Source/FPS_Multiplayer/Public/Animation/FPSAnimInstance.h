// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "TurnInPlaceAnimInterface.h"
#include "TurnInPlaceStatics.h"
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
enum class ELocomotionCardinalDirection : uint8
{
    LSD_Forward    UMETA(DisplayName = "Forward"),
    LSD_Right      UMETA(DisplayName = "Right"),
    LSD_Left       UMETA(DisplayName = "Left"),
    LSD_Backward   UMETA(DisplayName = "Backward"),
    LSD_MAX        UMETA(Hidden)
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
class FPS_MULTIPLAYER_API UFPSAnimInstance : public UAnimInstance, public ITurnInPlaceAnimInterface
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
    
    // --- TURN IN PLACE INTERFACE ---
    virtual FTurnInPlaceAnimSet GetTurnInPlaceAnimSet_Implementation() const override
    {
        if (bIsCrouching)
        {
            return TurnInPlaceAnimSetCrouched;
        } 
        return TurnInPlaceAnimSet;
    };
    
    virtual FTurnInPlaceCurveValues GetTurnInPlaceCurveValues_Implementation() const override
    {
        return TurnInPlaceCurveValues;
    };
    
protected:
    
    // =========================================================================
    //                        ESSENTIAL DATA (Gathered)
    // =========================================================================
    
    /** Current velocity vector in World Space. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    FVector WorldVelocity;
    
    /** Current velocity ignoring Z axis (useful for ground speed). */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    FVector WorldVelocity2D;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    FVector LocalVelocity2D;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    float LocalVelocityDirectionAngle;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    float LocalVelocityDirectionAngleWithOffset;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    ELocomotionCardinalDirection LocalVelocityDirection;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    ELocomotionCardinalDirection LocalVelocityDirectionNoOffset;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    bool bHasVelocity;
    
    /** The smoothed angle fed directly into the Orientation Warping node */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    float SmoothedLocomotionAngle;
    
    /** Current acceleration (Input intent) from the Movement Component. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Acceleration Data")
    FVector WorldAcceleration;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Acceleration Data")
    FVector WorldAcceleration2D;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Acceleration Data")
    FVector LocalAcceleration2D;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Acceleration Data")
    bool bHasAcceleration;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsOnGround;
    /** Gate: True if player has input AND speed > Threshold. Prevents micro-movements. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bShouldMove; 
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsJumping;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    float TimeToJumpApex;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsFalling;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsCrouching;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bCrouchStateChanged;
    
    /** Helper for AnimGraph: Are we in the Sprinting State? */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsSprinting;
    
    /** Helper for AnimGraph: Are we Aiming Down Sights? */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsAiming;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsArmed;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsLocallyControlled;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsRunningIntoWall;
    
    /** The "Source of Truth" replicated from the Character (Gait, Stance, etc.). */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    FCharacterLayerStates LayerStates;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Location Data")
    float DisplacementSpeed;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Location Data")
    float DisplacementSinceLastUpdate;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Location Data")
    float Speed3D;
    
    /** Speed across the ground (cm/s). Drives blendspaces. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Location Data")
    float GroundSpeed;
    
    // =========================================================================
    //                   TURN IN PLACE & ROTATION
    // =========================================================================
    
    // Internal Turn In Place Variables
    
    // This holds your Step Sizes, Angles, and Animation Assets.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place")
    FTurnInPlaceAnimSet TurnInPlaceAnimSet;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place")
    FTurnInPlaceAnimSet TurnInPlaceAnimSetCrouched;
    
    // This Struct holds all the decisions: "bWantsToTurn", "StepSize", "TurnAngle", etc.
    UPROPERTY(Transient, BlueprintReadWrite, Category = "Turn In Place")
    FTurnInPlaceAnimGraphData TurnInPlaceAnimGraphData;
    
    UPROPERTY(Transient, BlueprintReadWrite, Category = "Turn In Place")
    FTurnInPlaceAnimGraphOutput TurnInPlaceAnimGraphOutput;
    
    UPROPERTY(Transient, BlueprintReadWrite, Category = "Turn In Place")
    FTurnInPlaceGraphNodeData TurnInPlaceGraphNodeData;
    
    // Cache for Thread Safety
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Turn In Place")
    FTurnInPlaceCurveValues TurnInPlaceCurveValues;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Turn In Place")
    bool bIsStrafing = false;
    
    // STATE: Tracks if we hit the max angle during this specific turn. 
    // Must be in Header so it persists across frames.
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Turn In Place")
    bool bCanUpdateTurnInPlace;
    
    /** The actual value fed into the "Rotate Root Bone" node in AnimGraph */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Turn In Place")
    float RootYawOffset;
    
    // =========================================================================
    //                        AIM OFFSETS (Spine)
    // =========================================================================

    // This value comes from Control Rotation (Up/Down)
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aim Offset")
    float AimPitch;
    
    /** 
     * Distributed pitch value (Total Pitch / Num Bones). 
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
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    float TimeToAim;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    float TimeFromAim;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    FTransform CurrentHipFireOffset;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    float CurrentDistanceFromCamera;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Two Bone IK")
    FVector CurrentRightHandEffectorLocation;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Two Bone IK")
    FVector CurrentRightHandJointTargetLocation;
    
    /** Which sight slot are we using? (0 = Iron Sights, 1 = Red Dot, etc.) */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|State")
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
    
    /** How fast the legs twist to match the movement direction. Lower = Heavier/Smoother. */
    UPROPERTY(EditDefaultsOnly, Category = "Configuration|Movement")
    float LocomotionAngleInterpSpeed = 10.0f;
    
    UPROPERTY(BlueprintReadOnly, Category = "Configuration|Data")
    float GroundDistance = -1.0f;

private:
    // Internal location tracking for displacement calc
    FVector WorldLocation;
    FDelegateHandle OnWeaponEquippedDelegateHandle;
    float CardinalDirectionDeadZone = 10.f;

    // =========================================================================
    //                        INTERNAL FUNCTIONS
    // =========================================================================

    void UpdateReferences();
    void UpdateLocationData(float DeltaTime);
    void UpdateRotationData();
    void UpdateVelocityData(float DeltaTime);
    void UpdateAccelerationData();
    void UpdateCharacterState();
    void UpdateAimingData();
    void UpdateJumpFallData();
    void UpdateWallDetectionHeuristic();
    
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
    
    ELocomotionCardinalDirection SelectCardinalDirectionFromAngle(float NewAngle, float NewDeadZone, ELocomotionCardinalDirection CurrentDirection, bool bUseCurrentDirection) const;
    
public:
    
    // =========================================================================
    //                        THREADSAFE FUNCTIONS
    // =========================================================================
    
    UFUNCTION(BlueprintPure, Category = "References", meta=(BlueprintThreadSafe))
    FORCEINLINE UFPSCharacterMovementComponent* GetMovementComponent() const
    {
        return TryGetPawnOwner()->GetMovementComponent() ? Cast<UFPSCharacterMovementComponent>(TryGetPawnOwner()->GetMovementComponent()) : nullptr;
    }
    
    UFUNCTION(BlueprintPure, Category = "References", meta=(BlueprintThreadSafe))
    FORCEINLINE AFPSPlayerCharacter* GetFPSCharacter() const
    {
        return TryGetPawnOwner() ? Cast<AFPSPlayerCharacter>(TryGetPawnOwner()) : nullptr;
    }
    
    UFUNCTION(BlueprintPure, Category="Turn In Place", meta=(BlueprintThreadSafe))
    FORCEINLINE FTurnInPlaceAnimSet GetTurnAnimSet() { return TurnInPlaceAnimGraphData.AnimSet; }
    
    UFUNCTION(BlueprintPure, Category="Turn In Place", meta=(BlueprintThreadSafe))
    FORCEINLINE UAnimSequence* GetTurnAnimation(const bool bRecover) { return UTurnInPlaceStatics::GetTurnInPlaceAnimation(GetTurnAnimSet(), TurnInPlaceGraphNodeData, bRecover); }

    UFUNCTION(BlueprintPure, Category="Turn In Place", meta=(BlueprintThreadSafe))
    FORCEINLINE float GetAnimStateTimeNodeData() const { return TurnInPlaceGraphNodeData.AnimStateTime; }
    
    UFUNCTION(BlueprintCallable, Category="Turn In Place", meta=(BlueprintThreadSafe))
    FORCEINLINE void SetHasReachedMaxTurnAngleNodeData(bool NewHasReachedMaxTurnAngle) { TurnInPlaceGraphNodeData.bHasReachedMaxTurnAngle = NewHasReachedMaxTurnAngle; }
    
    UFUNCTION(BlueprintCallable, Category="Turn In Place", meta=(BlueprintThreadSafe))
    FORCEINLINE void SetStepSizeNodeData(int32 NewStepSize) { TurnInPlaceGraphNodeData.StepSize = NewStepSize; }
    
    UFUNCTION(BlueprintCallable, Category="Turn In Place", meta=(BlueprintThreadSafe))
    FORCEINLINE void SetIsTurningRightNodeData(bool NewIsTurningRight) { TurnInPlaceGraphNodeData.bIsTurningRight = NewIsTurningRight; }
    
    UFUNCTION(BlueprintCallable, Category="Turn In Place", meta=(BlueprintThreadSafe))
    FORCEINLINE void SetAnimStateTimeNodeData(float NewAnimStateTime) { TurnInPlaceGraphNodeData.AnimStateTime = NewAnimStateTime; }
    
    UFUNCTION(BlueprintPure, Category="Distance Matching", meta=(BlueprintThreadSafe))
    float GetPredictedStopDistance() const;
    
    UFUNCTION(BlueprintPure, Category="Distance Matching", meta=(BlueprintThreadSafe))
    bool ShouldDistanceMatchStop() const;
};