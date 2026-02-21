// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "TurnInPlaceAnimInterface.h"
#include "TurnInPlaceStatics.h"
#include "Animation/AnimInstance.h"
#include "Character/FPSPlayerCharacter.h"
#include "Data/Structs/FPSCharacterDataContainer.h"
#include "FPSAnimInstance.generated.h"

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
    
    // --- THE THREAD-SAFE SPLIT ---
    // 1. Game Thread: Gathers raw data from components. Safe to access UObjects (Character, MovementComponent, Weapons).
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    // 2. Worker Thread: Runs heavy math in parallel using cached data.
    virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
    
    // --- TURN IN PLACE INTERFACE ---
    virtual FTurnInPlaceAnimSet GetTurnInPlaceAnimSet_Implementation() const override
    {
        switch (LayerStates.OverlayState) 
        {
            case EOverlayState::EOS_Unarmed:
                if (bIsCrouching)
                {
                    return TurnInPlaceAnimSetCrouched;
                } 
                return TurnInPlaceAnimSet;
                
            case EOverlayState::EOS_Rifle:
            case EOverlayState::EOS_Shotgun:
                if (bIsCrouching)
                {
                    return TurnInPlaceRifleAnimSetCrouched;
                } 
                return TurnInPlaceRifleAnimSet;

            case EOverlayState::EOS_Pistol:
                if (bIsCrouching)
                {
                    return TurnInPlacePistolAnimSetCrouched;
                } 
                return TurnInPlacePistolAnimSet;
                
            case EOverlayState::EOS_Knife:
                break;
        }
        
        return FTurnInPlaceAnimSet();
    };
    
    virtual FTurnInPlaceCurveValues GetTurnInPlaceCurveValues_Implementation() const override
    {
        return TurnInPlaceCurveValues;
    };
    
protected:
    
    // =========================================================================
    //                   THREAD-SAFE CACHED DATA (Game Thread -> Worker Thread)
    // =========================================================================
    
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
    
    /** The target transform calculated on Game Thread, smoothed on Worker Thread */
    UPROPERTY(Transient) FTransform DesiredHandTransformTarget;
    
    // =========================================================================
    //                        ESSENTIAL DATA (Calculated)
    // =========================================================================
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    FVector WorldVelocity;
    
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
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Velocity Data")
    float SmoothedLocomotionAngle;
    
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
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsSprinting;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsAiming;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsArmed;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsLocallyControlled;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
    bool bIsRunningIntoWall;
    
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
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place | Unarmed")
    FTurnInPlaceAnimSet TurnInPlaceAnimSet;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place | Unarmed")
    FTurnInPlaceAnimSet TurnInPlaceAnimSetCrouched;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place | Rifle")
    FTurnInPlaceAnimSet TurnInPlaceRifleAnimSet;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place | Rifle")
    FTurnInPlaceAnimSet TurnInPlaceRifleAnimSetCrouched;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place | Pistol")
    FTurnInPlaceAnimSet TurnInPlacePistolAnimSet;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place | Pistol")
    FTurnInPlaceAnimSet TurnInPlacePistolAnimSetCrouched;
    
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
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Turn In Place")
    float RootYawOffset;

   // =========================================================================
    //                        AIM OFFSETS & ADS
    // =========================================================================

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aim Offset")
    float AimPitch;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "AimOffset")
    FRotator PitchValuePerBone;
    
    UPROPERTY(Transient, EditDefaultsOnly, Category = "Aiming")
    float AimInterpSpeed = 15.0f;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    TArray<FTransform> HandTransforms;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    FTransform CurrentHandTransform;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    FVector HandLocation;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    FRotator HandRotation;
    
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
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|State")
    int32 CurrentSightIndex = 0;
    
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
    //                           CONFIGURATION
    // =========================================================================
    
    UPROPERTY(EditDefaultsOnly, Category = "Configuration|Skeleton")
    FName HandBoneName = FName("hand_r");

    UPROPERTY(EditDefaultsOnly, Category = "Configuration|Skeleton")
    FName HeadBoneName = FName("head");
    
    UPROPERTY(EditDefaultsOnly, Category = "Configuration|AimOffset")
    float PitchPerBoneInterpSpeed = 10.0f;
    
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
    
    // Game Thread: Gathers socket and component transforms
    void GatherLeftHandTransform();
    void GatherProceduralAimingTarget();
    
    // Worker Thread: Thread-safe math updates
    void UpdateLocationData(float DeltaTime);
    void UpdateRotationData();
    void UpdateVelocityData(float DeltaTime);
    void UpdateAccelerationData();
    void UpdateCharacterState();
    void UpdateAimingData(float DeltaTime);
    void UpdateJumpFallData();
    void UpdateWallDetectionHeuristic();
    void UpdateProceduralAimingInterp(float DeltaTime);
    
    UFUNCTION()
    void OnCharacterWeaponEquipped(AFPSWeapon* NewWeapon);
    
    ELocomotionCardinalDirection SelectCardinalDirectionFromAngle(float NewAngle, float NewDeadZone, ELocomotionCardinalDirection CurrentDirection, bool bUseCurrentDirection) const;
    
public:
    
    // =========================================================================
    //                        THREADSAFE FUNCTIONS
    // =========================================================================
    
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