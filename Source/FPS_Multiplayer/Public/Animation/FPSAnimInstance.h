// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
/*#include "TurnInPlaceAnimInterface.h"
#include "TurnInPlaceStatics.h"*/
#include "Animation/AnimInstance.h"
#include "Character/FPSPlayerCharacter.h"
#include "Data/Structs/FPSCharacterDataContainer.h"
#include "FPSAnimInstance.generated.h"

#pragma region Structs

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

#pragma endregion Enums

UCLASS()
class FPS_MULTIPLAYER_API UFPSAnimInstance : public UAnimInstance //public ITurnInPlaceAnimInterface
{
    GENERATED_BODY()
    
public:
    
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUninitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
    
    //virtual void GetTurnInPlaceAnimSet_Implementation() const;
    
    virtual void GetTurnInPlaceCurveValues_Implementation() const
    {
    };
    
protected:
    
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
    
    UPROPERTY(Transient) FTransform DesiredHandTransformTarget;
    UPROPERTY(Transient) FTransform HeadBoneTransform;
    
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
    
    /*UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turn In Place")
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
    FTurnInPlaceCurveValues TurnInPlaceCurveValues;*/
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Turn In Place")
    bool bIsStrafing = false;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Turn In Place")
    bool bCanUpdateTurnInPlace;
   
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Turn In Place")
    float RootYawOffset;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "AimOffset")
    float AimPitch;
   
    UPROPERTY(Transient, BlueprintReadOnly, Category = "AimOffset")
    FRotator RotationValuePerBone;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    TArray<FTransform> HandTransforms;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    FVector HandLocation;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    FRotator HandRotation;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
    FTransform RightHandTargetTransform;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Positioning")
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
    FVector RightElbowJointLocation;
    
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Aiming|Two Bone IK")
    FVector LeftElbowJointLocation;
    
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
  
    UPROPERTY(EditAnywhere, Category = "Configuration|AimOffset")
    bool bDrawProxyAimDebug = true;
    
    UPROPERTY(EditDefaultsOnly, Category = "Configuration|AimOffset")
    float RotationPerBoneInterpSpeed = 10.0f;
    
    UPROPERTY(Transient, VisibleAnywhere, Category = "Configuration|AimOffset")
    float GunHorizontalOffsetCM;
    
    UPROPERTY(Transient, VisibleAnywhere, Category = "Configuration|AimOffset")
    float GunVerticalOffsetCM;
    
    UPROPERTY(Transient, VisibleAnywhere, Category = "Configuration|AimOffset")
    float GunPitchZeroingAngle; // Negative usually points the barrel down
    
    UPROPERTY(Transient, VisibleAnywhere, Category = "Configuration|AimOffset")
    float GunYawZeroingAngle;
    
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
    
    UPROPERTY(Transient, EditDefaultsOnly, Category = "Configuration|Aiming")
    float AimInterpSpeed = 15.0f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Configuration|Movement")
    float LocomotionAngleInterpSpeed = 15.0f;
    
    UPROPERTY(Transient) 
    float CachedAimDistance = 20000.f;
    
    FVector WorldLocation;
    FDelegateHandle OnWeaponEquippedDelegateHandle;
    float CardinalDirectionDeadZone = 10.f;
    FRotator SmoothedControlRotation;
    FRotator SmoothedActorRotation;
    FTransform SmoothedHandWorldTransform;
    
#pragma region Internal Gather Methods
    // --- Update Subroutines ---
    void UpdateReferences();
    void GatherLeftHandTransform();
    void GatherProceduralAimingTarget();
    
    UFUNCTION()
    void OnCharacterWeaponEquipped(AFPSWeapon* NewWeapon);
#pragma endregion
    
#pragma region Internal Worker Methods
    void UpdateLocationData(float DeltaTime);
    void UpdateRotationData(float DeltaTime);
    void UpdateVelocityData(float DeltaTime);
    void UpdateAccelerationData();
    void UpdateCharacterState();
    void UpdateAimingData(float DeltaTime);
    void UpdateJumpFallData();
    void UpdateWallDetectionHeuristic();
    void UpdateProceduralAimingInterp(float DeltaTime);
#pragma endregion
    
    ELocomotionCardinalDirection SelectCardinalDirectionFromAngle(float NewAngle, float NewDeadZone, ELocomotionCardinalDirection CurrentDirection, bool bUseCurrentDirection) const; 

public:
    
    /*UFUNCTION(BlueprintPure, Category="Turn In Place", meta=(BlueprintThreadSafe))
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
    FORCEINLINE void SetAnimStateTimeNodeData(float NewAnimStateTime) { TurnInPlaceGraphNodeData.AnimStateTime = NewAnimStateTime; }*/
    
    UFUNCTION(BlueprintPure, Category="Distance Matching", meta=(BlueprintThreadSafe))
    float GetPredictedStopDistance() const;

    UFUNCTION(BlueprintPure, Category="Distance Matching", meta=(BlueprintThreadSafe))
    bool ShouldDistanceMatchStop() const;
    
protected:
  
    UFUNCTION()
    void AnimNotify_WeaponGrab();
    
    void SetRootYawOffset(float InRootYawOffset);
    
    bool bEnableRootYawOffset = true;
    float RootTurnYawOffset;
    float AimYaw;
};