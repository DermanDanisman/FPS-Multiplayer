// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimExecutionContext.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimNodeReference.h"
#include "FPSItemAnimLayerBase.generated.h"

#pragma region Structs

class UFPSAnimInstanceBase;
class UAimOffsetBlendSpace;

USTRUCT(BlueprintType)
struct FAnimCardinalDirections
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

/**
 * 
 */
UCLASS()
class FPS_MULTIPLAYER_API UFPSItemAnimLayerBase : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
	/*UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ThreadSafe Getters", meta=(BlueprintThreadSafe))
	virtual UFPSAnimInstanceBase* GetFPSAnimInstance() const;*/
	
	UFUNCTION(BlueprintCallable, Category = "Animation|Node Functions", meta = (BlueprintThreadSafe))
	void SetLeftHandPoseOverrideWeight(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	
	// A proxy function so the AnimGraph can safely set the pivot time on the Base Instance
	UFUNCTION(BlueprintCallable, Category = "Turn In Place", meta=(BlueprintThreadSafe))
	void SetBaseLastPivotTime(float InTime);
	
protected:
	
	// Add a variable to cache the parent instance
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Anim Instances")
	TObjectPtr<UFPSAnimInstanceBase> BaseAnimInstance;
	
	//
	// --- Thread-Safe Data ---
	//
	
	UPROPERTY(BlueprintReadOnly, Category = "Distance Matching")
	float PredictedStopDistance;
	
	UPROPERTY(BlueprintReadOnly, Category = "Distance Matching")
	bool bShouldEnableFootPlacement;
	
	//
	// --- Data ---
	//
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Idle")
	TObjectPtr<UAnimSequence> IdleADS;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Idle")
	TObjectPtr<UAnimSequence> IdleHipFire;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Idle")
	TArray<UAnimSequence*> IdleBreaks;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Idle")
	TObjectPtr<UAnimSequence> CrouchIdle;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Idle")
	TObjectPtr<UAnimSequence> CrouchIdleEntry;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Idle")
	TObjectPtr<UAnimSequence> CrouchIdleExit;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Idle")
	TObjectPtr<UAnimSequence> LeftHandPose_Override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Starts")
	FAnimCardinalDirections JogStartCardinals;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Starts")
	FAnimCardinalDirections ADSStartCardinals;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Starts")
	FAnimCardinalDirections CrouchStartCardinals;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Stops")
	FAnimCardinalDirections JogStopCardinals;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Stops")
	FAnimCardinalDirections ADSStopCardinals;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Stops")
	FAnimCardinalDirections CrouchStopCardinals;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Pivots")
	FAnimCardinalDirections JogPivotCardinals;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Pivots")
	FAnimCardinalDirections ADSPivotCardinals;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Pivots")
	FAnimCardinalDirections CrouchPivotCardinals;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Turn In Place")
	TObjectPtr<UAnimSequence> TurnInPlaceLeft;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Turn In Place")
	TObjectPtr<UAnimSequence> TurnInPlaceRight;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Turn In Place")
	TObjectPtr<UAnimSequence> CrouchTurnInPlaceLeft;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Turn In Place")
	TObjectPtr<UAnimSequence> CrouchTurnInPlaceRight;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Jog")
	FAnimCardinalDirections JogCardinals;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Jump")
	TObjectPtr<UAnimSequence> JumpStart;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Jump")
	TObjectPtr<UAnimSequence> JumpApex;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Jump")
	TObjectPtr<UAnimSequence> JumpFallLand;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Jump")
	TObjectPtr<UAnimSequence> JumpRecoveryAdditive;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Jump")
	TObjectPtr<UAnimSequence> JumpStartLoop;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Jump")
	TObjectPtr<UAnimSequence> JumpFallLoop;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Jump")
	FName JumpDistanceCurveName = FName("GroundDistance");
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Walk")
	FAnimCardinalDirections WalkCardinals;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Walk")
	FAnimCardinalDirections CrouchWalkCardinals;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Aiming | FPS")
	TObjectPtr<UAnimSequence> AimFPSPelvisPose;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Aiming | FPS")
	TObjectPtr<UAnimSequence> AimFPSPelvisPoseCrouch;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Aiming")
	TObjectPtr<UAnimSequence> AimHipFirePose;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Aiming")
	TObjectPtr<UAnimSequence> AimHipFirePoseCrouch;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Aiming")
	TObjectPtr<UAimOffsetBlendSpace> IdleAimOffset;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Set Aiming")
	TObjectPtr<UAimOffsetBlendSpace> RelaxedAimOffset;
	
	UPROPERTY(BlueprintReadWrite, Category = "Blend Weight Data")
	float HipFireUpperBodyOverrideWeight;
	
	UPROPERTY(BlueprintReadWrite, Category = "Blend Weight Data")
	float AimOffsetBlendWeight;
	
	UPROPERTY(BlueprintReadWrite, Category = "Turn In Place")
	float TurnInPlaceAnimTime;
	
	UPROPERTY(BlueprintReadWrite, Category = "Turn In Place")
	float TurnInPlaceRotationDirection;
	
	UPROPERTY(BlueprintReadWrite, Category = "Turn In Place")
	float TurnInPlaceRecoveryDirection;
	
	UPROPERTY(BlueprintReadWrite, Category = "Idle Breaks")
	bool bWantsIdleBreak;
	
	UPROPERTY(BlueprintReadWrite, Category = "Idle Breaks")
	float TimeUntilNextIdleBreak;
	
	UPROPERTY(BlueprintReadWrite, Category = "Idle Breaks")
	int32 CurrentIdleBreakIndex;
	
	UPROPERTY(BlueprintReadWrite, Category = "Idle Breaks")
	float IdleBreakDelayTime;
	
	UPROPERTY(BlueprintReadWrite, Category = "Pivots")
	FVector PivotStartingAcceleration;
	
	UPROPERTY(BlueprintReadWrite, Category = "Pivots")
	float TimeAtPivotStop;
	
	UPROPERTY(BlueprintReadWrite, Category = "Jump")
	float LandRecoveryAlpha;
	
	UPROPERTY(BlueprintReadWrite, Category = "Jump")
	float TimeFalling;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skeleton Control Data")
	float HandIK_RightAlpha = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skeleton Control Data")
	float HandIK_LeftAlpha = 1.0f;
	
	UPROPERTY(BlueprintReadWrite, Category = "Stride Warping")
	float StrideWarpingStartAlpha;
	
	UPROPERTY(BlueprintReadWrite, Category = "Stride Warping")
	float StrideWarpingPivotAlpha;
	
	UPROPERTY(BlueprintReadWrite, Category = "Stride Warping")
	float StrideWarpingCycleAlpha;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FVector2D PlayRateClampStartsPivots = FVector2D(0.6f, 5.0f);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	bool bRaiseWeaponAfterFiringWhenCrouched;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	bool bDisableHandIK;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	bool bEnableLeftHandPoseOverride;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	float RaiseWeaponAfterFiringDuration = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	float StrideWarpingBlendInDurationScaled = 0.2f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	float StrideWarpingBlendInStartOffset = 0.15f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FName LocomotionDistanceCurveName = FName("Distance");
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FVector2D PlayRateClampCycle = FVector2D(0.8f, 1.2f);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS")
	float Alpha01_spine_01 = 0.15f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS")
	float Alpha02_spine_02 = 0.1f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS")
	float Alpha03_spine_03 = 0.1f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS")
	float Alpha04_spine_04 = 0.1f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS")
	float Alpha05_spine_05 = 0.1f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS")
	float Alpha06_neck_01 = 0.15f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS")
	float Alpha07_neck_02 = 0.2f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS")
	float Alpha08_head = 0.1f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS")
	TObjectPtr<UBlendSpace> FPS_IdleMovementBlendSpace;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS")
	TObjectPtr<UBlendSpace> FPS_ADSIdleMovementBlendSpace;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS")
	TObjectPtr<UAnimSequenceBase> FPS_JumpIdle;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FPS")
	TObjectPtr<UAnimSequenceBase> FPS_JumpADSIdle;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Instance")
	float LeftHandPoseOverrideWeight = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim Instance")
	float HandFKWeight = 0.0f;
	
private:
	
	//
	// --- Thread-Safe Functions
	//
	
	void UpdateBlendWeightData(float DeltaTime);
	void UpdateJumpFallData(float DeltaTime);
	void UpdateSkeletalControlData();
	void UpdatePredictedStopDistance();
};
