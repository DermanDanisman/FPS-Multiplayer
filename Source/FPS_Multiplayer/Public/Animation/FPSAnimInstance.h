// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "TurnInPlaceAnimInterface.h"
#include "TurnInPlaceStatics.h"
#include "Animation/AnimInstance.h"
#include "Character/FPSPlayerCharacter.h"
#include "Data/Structs/FPSCharacterDataContainer.h"
#include "FPSAnimInstance.generated.h"

#pragma region Structs
/** 
 * Container for directional locomotion animations.
 * Grouping these into a struct keeps the detail panel clean and makes it easy
 * to pass an entire set of directional data (Walk, Jog, Crouch) into utility functions.
 */
USTRUCT(BlueprintType)
struct FLocomotionAnimCardinalDirections
{
    GENERATED_BODY()
	
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Locomotion Cardinal Directions")
    TObjectPtr<UAnimSequence> Forward;
	
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Locomotion Cardinal Directions")
    TObjectPtr<UAnimSequence> Backward;
	
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Locomotion Cardinal Directions")
    TObjectPtr<UAnimSequence> Left;
	
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Locomotion Cardinal Directions")
    TObjectPtr<UAnimSequence> Right;
};
#pragma endregion Structs

#pragma region Enums
/**
 * Determines which specific directional animation to play when transitioning states 
 * (e.g., which start-walk animation to play when moving from Idle). 
 */
UENUM(BlueprintType)
enum class ELocomotionCardinalDirection : uint8
{
    LSD_Forward    UMETA(DisplayName = "Forward"),
    LSD_Backward   UMETA(DisplayName = "Backward"),
    LSD_Left       UMETA(DisplayName = "Left"),
    LSD_Right      UMETA(DisplayName = "Right"),
    
    LSD_MAX        UMETA(Hidden)
};
#pragma endregion Enums

/**
 * Main Animation Instance for the First Person Character.
 * * ARCHITECTURE OVERVIEW: "Gather-Read" (Thread-Safe)
 * Unreal Engine 5 heavily utilizes multi-threading for animations to save CPU time.
 * We must strictly separate our logic into two phases:
 * 1. GAME THREAD (NativeUpdateAnimation): Safe to access Actors, Components, and the World.
 * We 'Gather' raw data here and cache it in Transient variables.
 * 2. WORKER THREAD (NativeThreadSafeUpdateAnimation): Runs in parallel. NOT safe to access Actors.
 * We 'Read' the cached variables here to perform heavy math (Trigonometry, IK targets, Velocity).
 */
UCLASS()
class FPS_MULTIPLAYER_API UFPSAnimInstance : public UAnimInstance, public ITurnInPlaceAnimInterface
{
    GENERATED_BODY()
    
public:
    // =========================================================================
    //                           LIFECYCLE
    // =========================================================================
    
    /** Called once when the AnimInstance is first created. Setup pointers and initial locations here. */
    virtual void NativeInitializeAnimation() override;
    
    /** Called when the AnimInstance is being destroyed. Good for cleaning up delegates/bindings. */
    virtual void NativeUninitializeAnimation() override;
    
    /** 
     * GAME THREAD EXECUTION: Runs every frame.
     * Rule: Only gather data here. Do not do heavy math. Store data in 'Cached' variables.
     */
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    
    /** 
     * WORKER THREAD EXECUTION: Runs every frame in parallel with other game logic.
     * Rule: Do NOT call GetOwningActor() or access components here. Only use Cached variables.
     */
    virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
    
    // --- TURN IN PLACE INTERFACE ---
    virtual FTurnInPlaceAnimSet GetTurnInPlaceAnimSet_Implementation() const override;
    
    virtual FTurnInPlaceCurveValues GetTurnInPlaceCurveValues_Implementation() const override
    {
        return TurnInPlaceCurveValues;
    };
    
protected:
    
    // =========================================================================
    //                   THREAD-SAFE CACHED DATA (Game -> Worker)
    // =========================================================================
    // 'Transient' means these variables are not saved to disk. They only exist at runtime.
    
    UPROPERTY(Transient) FVector CachedActorLocation;
    UPROPERTY(Transient) FRotator CachedActorRotation;
    UPROPERTY(Transient) FVector CachedVelocity;
    UPROPERTY(Transient) FVector CachedAcceleration;
    UPROPERTY(Transient) FRotator CachedControlRotation;
    UPROPERTY(Transient) FVector CachedLastUpdateVelocity;

    UPROPERTY(Transient) float CachedGravityZ;
    UPROPERTY(Transient) float CachedMaxWalkSpeed;
    UPROPERTY(Transient) float CachedCurrentWalkSpeed;
    UPROPERTY(Transient) float CachedBrakingFriction;
    UPROPERTY(Transient) float CachedGroundFriction;
    UPROPERTY(Transient) float CachedBrakingFrictionFactor;
    UPROPERTY(Transient) float CachedBrakingDecelerationWalking;

    UPROPERTY(Transient) TEnumAsByte<EMovementMode> CachedMovementMode;
    UPROPERTY(Transient) bool bCachedIsMovingOnGround;
    UPROPERTY(Transient) bool bCachedIsCrouching;
    UPROPERTY(Transient) bool bCachedOrientRotationToMovement;
    UPROPERTY(Transient) bool bCachedUseSeparateBrakingFriction;
    
    /*UPROPERTY(Transient) float CachedGunHorizontalOffsetCM;
    UPROPERTY(Transient) float CachedGunVerticalOffsetCM;
    UPROPERTY(Transient) float CachedGunPitchZeroingAngle;
    UPROPERTY(Transient) float CachedGunYawZeroingAngle;*/
    
    /** The target IK hand transform calculated safely on the Game Thread, smoothed on the Worker Thread. */
    UPROPERTY(Transient) FTransform DesiredHandTransformTarget;
    
    // =========================================================================
    //                        ESSENTIAL DATA (Calculated)
    // =========================================================================
    // These are the final values read directly by the Animation Blueprint graph.
    
    /** Actual movement velocity in 3D world space. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    FVector WorldVelocity;
    
    /** Movement velocity ignoring the Z (Up/Down) axis. Useful for ground speed. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    FVector WorldVelocity2D;
    
    /** Velocity relative to the character's facing direction (e.g., X is always forward). */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    FVector LocalVelocity2D;
    
    /** The angle of movement relative to where the character is looking. (-180 to 180) */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    float LocalVelocityDirectionAngle;
    
    /** The movement angle adjusted for the current Aim Offset Yaw twist. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    float LocalVelocityDirectionAngleWithOffset;
    
    /** Discretized direction (Forward, Backward, Left, Right) used to pick the correct animation sequence. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    ELocomotionCardinalDirection LocalVelocityDirection;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    ELocomotionCardinalDirection LocalVelocityDirectionNoOffset;
    
    /** True if the character is currently moving faster than a near-zero threshold. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    bool bHasVelocity;
    
    /** Interped version of the locomotion angle to prevent blendspace snapping when changing directions. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    float SmoothedLocomotionAngle;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Acceleration Data")
    FVector WorldAcceleration;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Acceleration Data")
    FVector WorldAcceleration2D;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Acceleration Data")
    FVector LocalAcceleration2D;
    
    /** True if the player is currently pressing a movement key/stick. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Acceleration Data")
    bool bHasAcceleration;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsOnGround;
    
    /** True if the character has input AND is moving fast enough to trigger locomotion animations. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bShouldMove; 
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsJumping;
    
    /** Calculated time remaining until the character reaches the highest point of their jump arc. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    float TimeToJumpApex;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsFalling;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsCrouching;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bCrouchStateChanged;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsSprinting;
    
    /** True if holding the Aim Down Sights input. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsAiming;
    
    /** True if a valid weapon is currently equipped in the hands. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsArmed;
    
    /** True if this machine owns the character. False if this is another player over the network. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsLocallyControlled;
    
    /** Heuristic flag true if the player is pressing forward but velocity is blocked by geometry. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsRunningIntoWall;
    
    /** Data struct representing the character's current gameplay layer (Unarmed, Rifle, Pistol, etc.) */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    FCharacterLayerStates LayerStates;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Location Data")
    float DisplacementSpeed;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Location Data")
    float DisplacementSinceLastUpdate;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Location Data")
    float Speed3D;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Location Data")
    float GroundSpeed;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Essential Data")
    FVector InputVector;
    
    // =========================================================================
    //                        LOCOMOTION MATH
    // =========================================================================
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
    FRotator StartRotation;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
    FRotator PrimaryRotation;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
    FRotator SecondaryRotation;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
    float LocomotionStartAngle;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
    ELocomotionCardinalDirection LocomotionStartDirection;
    
    // =========================================================================
    //                   TURN IN PLACE & ROTATION
    // =========================================================================
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place")
    FTurnInPlaceAnimSet TurnInPlaceAnimSet;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place")
    FTurnInPlaceAnimSet TurnInPlaceAnimSetCrouched;
    
    UPROPERTY(Transient, BlueprintReadWrite, Category = "Turn In Place")
    FTurnInPlaceAnimGraphData TurnInPlaceAnimGraphData;
    
    UPROPERTY(Transient, BlueprintReadWrite, Category = "Turn In Place")
    FTurnInPlaceAnimGraphOutput TurnInPlaceAnimGraphOutput;
    
    UPROPERTY(Transient, BlueprintReadWrite, Category = "Turn In Place")
    FTurnInPlaceGraphNodeData TurnInPlaceGraphNodeData;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Turn In Place")
    FTurnInPlaceCurveValues TurnInPlaceCurveValues;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Turn In Place")
    bool bIsStrafing = false;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Turn In Place")
    bool bCanUpdateTurnInPlace;
    
    /** The difference between the camera's looking direction and the capsule's facing direction. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Turn In Place")
    float RootYawOffset;

    // =========================================================================
    //                        AIM OFFSETS & PROCEDURAL AIMING
    // =========================================================================

    /** The vertical angle difference between the camera and the capsule. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aim Offset")
    float AimPitch;
    
    /** Final calculated FRotator to apply per spine bone in the AnimGraph for procedural aiming. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "AimOffset")
    FRotator RotationValuePerBone;
    
    /** Cached array of perfectly aligned hand transforms for every sight on the current weapon. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    TArray<FTransform> HandTransforms;
    
    /** The current smoothly interpolating hand transform (used by IK). */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    FTransform CurrentHandTransform;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    FVector HandLocation;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    FRotator HandRotation;
    
    /** The target transform for the left hand, calculated dynamically based on weapon sockets. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|FABRIK")
    FTransform LeftHandTargetTransform;
    
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
    
    /** Which sight index is currently active (e.g., 0 = Red Dot, 1 = Canted Irons). */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|State")
    int32 CurrentSightIndex = 0;
    
    /** Highly optimized data struct to cache sight socket data when a weapon is equipped. */
    struct FCachedSightData
    {
       bool bIsOptic; 
       int32 SightIndex; 
       TWeakObjectPtr<UMeshComponent> Mesh; 
       FName SocketNameA; 
       FName SocketNameB; 
       TWeakObjectPtr<UMeshComponent> RearMesh; 
       
       bool operator<(const FCachedSightData& Other) const { return SightIndex < Other.SightIndex; }
    };

    TArray<FCachedSightData> CachedSights;
    
    // =========================================================================
    //                  PARALLAX & CONVERGENCE CONFIGURATION
    // =========================================================================
    
    UPROPERTY(Transient, VisibleAnywhere, Category = "Configuration|AimOffset")
    float GunHorizontalOffsetCM;
    
    UPROPERTY(Transient, VisibleAnywhere, Category = "Configuration|AimOffset")
    float GunVerticalOffsetCM;

    /**
     * If true, draws debug lines and spheres representing the simulated proxy's line of sight.
     * Green Line/Red Sphere = A valid hit. The weapon will converge exactly on the red sphere.
     * Red Line = No hit (aiming at the sky). The weapon will aim perfectly parallel.
     */
    UPROPERTY(EditAnywhere, Category = "Configuration|AimOffset")
    bool bDrawProxyAimDebug = true;
    
    UPROPERTY(Transient, VisibleAnywhere, Category = "Configuration|AimOffset")
    float GunPitchZeroingAngle; // Negative usually points the barrel down
    
    UPROPERTY(Transient, VisibleAnywhere, Category = "Configuration|AimOffset")
    float GunYawZeroingAngle;
    
    // =========================================================================
    //                           CONFIGURATION (Anim Sequences)
    // =========================================================================
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Idle")
    TObjectPtr<UAnimSequence> Idle;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Idle")
    TObjectPtr<UAnimSequence> IdleADS;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Idle")
    TObjectPtr<UAnimSequence> IdleHipFire;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Idle")
    TObjectPtr<UAnimSequence> CrouchIdle;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Idle")
    TObjectPtr<UAnimSequence> CrouchIdleEntry;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Idle")
    TObjectPtr<UAnimSequence> CrouchIdleExit;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Starts|Walk")
    FLocomotionAnimCardinalDirections WalkStart;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Starts|Jog")
    FLocomotionAnimCardinalDirections JogStart;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Starts|Crouch")
    FLocomotionAnimCardinalDirections CrouchStart;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Cycles|Walk")
    FLocomotionAnimCardinalDirections WalkCycle;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Cycles|Jog")
    FLocomotionAnimCardinalDirections JogCycle;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Cycles|Crouch")
    FLocomotionAnimCardinalDirections CrouchCycle;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Stops|Walk")
    FLocomotionAnimCardinalDirections WalkStop;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Stops|Jog")
    FLocomotionAnimCardinalDirections JogStop;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Stops|Crouch")
    FLocomotionAnimCardinalDirections CrouchStop;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Jump")
    TObjectPtr<UAnimSequence> JumpStart;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Jump")
    TObjectPtr<UAnimSequence> JumpApex;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Jump")
    TObjectPtr<UAnimSequence> JumpFallLand;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Jump")
    TObjectPtr<UAnimSequence> JumpStartLoop;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Jump")
    TObjectPtr<UAnimSequence> JumpFallLoop;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Aiming")
    TObjectPtr<UAnimSequence> HipFirePose;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation Set|Aiming")
    TObjectPtr<UAnimSequence> ADSFirePose;
    
    UPROPERTY(BlueprintReadOnly, Category = "Configuration|Data")
    float GroundDistance = -1.0f;

private:
    
    UPROPERTY(EditDefaultsOnly, Category = "Configuration|Skeleton")
    FName HandBoneName = FName("hand_r");

    UPROPERTY(EditDefaultsOnly, Category = "Configuration|Skeleton")
    FName HeadBoneName = FName("head");
    
    UPROPERTY(EditDefaultsOnly, Category = "Configuration|AimOffset")
    float RotationPerBoneInterpSpeed = 10.0f;
    
    UPROPERTY(Transient, EditDefaultsOnly, Category = "Configuration|Aiming")
    float AimInterpSpeed = 15.0f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Configuration|Movement")
    float LocomotionAngleInterpSpeed = 10.0f;
    
    // =========================================================================
    //                  THREAD-SAFE CACHED DATA
    // =========================================================================

    /** * The physical distance to the surface the simulated proxy is currently looking at.
     * Calculated via a zero-cost Line Trace on the Game Thread. 
     * Read by the Worker Thread to compute the trigonometric convergence angles (Atan2) 
     * for the procedural Aim Offset correction. Defaults to 20000.f (Infinity/Parallel).
     */
    UPROPERTY(Transient) 
    float CachedAimDistance = 20000.f;
    
    // Internal location tracking for displacement calc
    FVector WorldLocation;
    FDelegateHandle OnWeaponEquippedDelegateHandle;
    
    /** Degree boundary to prevent rapid switching between locomotion animations (e.g., Walk Forward vs Walk Right) */
    float CardinalDirectionDeadZone = 10.f;
    
    /** Interpolated rotation representing where the player is aiming. */
    FRotator SmoothedControlRotation;
    
    /** Interpolated rotation representing where the physical capsule is facing. */
    FRotator SmoothedActorRotation;

    // =========================================================================
    //                        INTERNAL FUNCTIONS
    // =========================================================================

    /** Grabs the character and binds combat delegates. */
    void UpdateReferences();
    
    /** GAME THREAD: Caches the left hand IK target based on the equipped weapon's socket. */
    void GatherLeftHandTransform();
    
    /** GAME THREAD: Calculates the perfect spatial transform to align weapon sights with the camera. */
    void GatherProceduralAimingTarget();
    
    /** WORKER THREAD: Calculates world and ground displacement/speed. */
    void UpdateLocationData(float DeltaTime);
    
    /** WORKER THREAD: Interpolates network rotations and calculates Yaw/Pitch deltas. */
    void UpdateRotationData(float DeltaTime);
    
    /** WORKER THREAD: Extracts directional angles and populates cardinal enums for blendspaces. */
    void UpdateVelocityData(float DeltaTime);
    
    /** WORKER THREAD: Calculates if the player is pressing movement input. */
    void UpdateAccelerationData();
    
    /** WORKER THREAD: Resolves jumping, falling, crouching, and aiming booleans. */
    void UpdateCharacterState();
    
    /** WORKER THREAD: Applies the Parallax Trigonometry to solve procedural spine IK. */
    void UpdateAimingData(float DeltaTime);
    
    /** WORKER THREAD: Solves the arc trajectory for jump animations. */
    void UpdateJumpFallData();
    
    /** WORKER THREAD: Simple heuristic to detect if the player is stuck against geometry. */
    void UpdateWallDetectionHeuristic();
    
    /** WORKER THREAD: Smoothly interps the procedural aim target gathered on the Game Thread. */
    void UpdateProceduralAimingInterp(float DeltaTime);
    
    /** Delegate fired when a weapon finishes equipping. Rebuilds sight caches. */
    UFUNCTION()
    void OnCharacterWeaponEquipped(AFPSWeapon* NewWeapon);
    
    /** Math utility to convert a 360-degree angle into a Cardinal Direction Enum with deadzones. */
    ELocomotionCardinalDirection SelectCardinalDirectionFromAngle(float NewAngle, float NewDeadZone, ELocomotionCardinalDirection CurrentDirection, bool bUseCurrentDirection) const; 

public:
    
    // =========================================================================
    //                        THREADSAFE FUNCTIONS
    // =========================================================================
    // These functions use the 'BlueprintThreadSafe' meta tag so they can be 
    // evaluated directly inside the AnimGraph nodes without causing warnings.
    
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
    
    /** Predicts exactly where the character will stop based on friction, allowing animations to match distance. */
    UFUNCTION(BlueprintPure, Category="Distance Matching", meta=(BlueprintThreadSafe))
    float GetPredictedStopDistance() const;
    
    /** Validates if the character is decelerating and should play a distance-matched stop animation. */
    UFUNCTION(BlueprintPure, Category="Distance Matching", meta=(BlueprintThreadSafe))
    bool ShouldDistanceMatchStop() const;
    
protected:
    
    /** * EVENT NOTIFY: Unreal Engine automatically binds this to any AnimNotify named "WeaponGrab" 
     * inside an animation sequence. Used to accurately time the weapon attachment to the hand.
     */
    UFUNCTION()
    void AnimNotify_WeaponGrab();
};