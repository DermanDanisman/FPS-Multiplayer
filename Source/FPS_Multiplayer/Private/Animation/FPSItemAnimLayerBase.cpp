// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "Animation/FPSItemAnimLayerBase.h"

#include "AnimCharacterMovementLibrary.h"
#include "SequenceEvaluatorLibrary.h"
#include "Animation/FPSAnimInstanceBase.h"


void UFPSItemAnimLayerBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UFPSItemAnimLayerBase::NativeUninitializeAnimation()
{
	Super::NativeUninitializeAnimation();
}

void UFPSItemAnimLayerBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
}

void UFPSItemAnimLayerBase::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	
	// ========================================================================
	// WORKER THREAD: Guard everything! If the Base isn't ready, do nothing.
	// ========================================================================
	if (!GetFPSAnimInstance()) return;

	// Curves are thread-safe, so we evaluate them here
	bShouldEnableFootPlacement = GetFPSAnimInstance()->GetCurveValue(FName("DisableLegIK")) <= 0.0f && GetFPSAnimInstance()->bUseFootPlacement; 

	// Run your layer's specific math functions safely
	UpdateBlendWeightData(DeltaSeconds);
	UpdateJumpFallData(DeltaSeconds);
	UpdateSkeletalControlData();
	UpdatePredictedStopDistance();
}

UFPSAnimInstanceBase* UFPSItemAnimLayerBase::GetFPSAnimInstance() const
{
	return Cast<UFPSAnimInstanceBase>(GetOwningComponent()->GetAnimInstance());
}

void UFPSItemAnimLayerBase::UpdateBlendWeightData(float DeltaTime)
{
	if ((!RaiseWeaponAfterFiringDuration && GetFPSAnimInstance()->bIsCrouching) || 
		   ((!GetFPSAnimInstance()->bIsCrouching && GetFPSAnimInstance()->bGameplayTagIsADS) && GetFPSAnimInstance()->bIsOnGround))
	{
		HipFireUpperBodyOverrideWeight = 0.0f;
		AimOffsetBlendWeight = 0.0f;
	}
	else
	{
		if ((GetFPSAnimInstance()->TimeSinceFiredWeapon < RaiseWeaponAfterFiringDuration) || 
		   (GetFPSAnimInstance()->bGameplayTagIsADS && (GetFPSAnimInstance()->bIsCrouching || !GetFPSAnimInstance()->bIsOnGround)) || 
		   GetCurveValue(FName("applyHipfireOverridePose")) > 0.0f)
		{
			HipFireUpperBodyOverrideWeight = 1.0f;
			AimOffsetBlendWeight = 1.0f;
		}
		else
		{
			HipFireUpperBodyOverrideWeight = FMath::FInterpTo(HipFireUpperBodyOverrideWeight, 0.0f, DeltaTime, 1.0f);
          
			if (FMath::Abs(GetFPSAnimInstance()->RootYawOffset) < 10.0f && GetFPSAnimInstance()->bHasAcceleration)
			{
				AimOffsetBlendWeight = FMath::FInterpTo(AimOffsetBlendWeight, HipFireUpperBodyOverrideWeight, DeltaTime, 10.0f);   
			}
			else
			{
				AimOffsetBlendWeight = FMath::FInterpTo(AimOffsetBlendWeight, 1.0f, DeltaTime, 10.0f); 
			}
		}
	}
}

void UFPSItemAnimLayerBase::UpdateJumpFallData(float DeltaTime)
{
	if (GetFPSAnimInstance()->bIsFalling)
	{
		TimeFalling = TimeFalling + DeltaTime;
	}
	else
	{
		if (GetFPSAnimInstance()->bIsJumping)
			TimeFalling = 0.0f;
	}
}

void UFPSItemAnimLayerBase::UpdateSkeletalControlData()
{
	float Substraction;
	if (bShouldDisableHandIK)
		Substraction = 0.0f;
	else
		Substraction = 1.0f;
	
	float RightAlphaValue = Substraction - GetCurveValue(FName("DisableRHandIK"));
	HandIK_RightAlpha = UKismetMathLibrary::Clamp(RightAlphaValue, 0.0f, 1.0f);
	
	float LeftAlphaValue = Substraction - GetCurveValue(FName("DisableLHandIK"));
	HandIK_LeftAlpha = UKismetMathLibrary::Clamp(LeftAlphaValue, 0.0f, 1.0f);
}

void UFPSItemAnimLayerBase::UpdatePredictedStopDistance()
{
	FVector LocalPredictedStopDistance = UAnimCharacterMovementLibrary::PredictGroundMovementStopLocation(
		GetFPSAnimInstance()->CachedLastUpdateVelocity, 
		GetFPSAnimInstance()->bCachedUseSeparateBrakingFriction,
		GetFPSAnimInstance()->CachedBrakingFriction,
		GetFPSAnimInstance()->CachedGroundFriction,
		GetFPSAnimInstance()->CachedBrakingFrictionFactor,
		GetFPSAnimInstance()->CachedBrakingDecelerationWalking
	);

	float NormalizedStopLocation = UKismetMathLibrary::VSizeXY(LocalPredictedStopDistance);
	PredictedStopDistance = NormalizedStopLocation;
}

void UFPSItemAnimLayerBase::SetLeftHandPoseOverrideWeight(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	float Value = bEnableLeftHandPoseOverride ? 1.0f : 0.0f;
	LeftHandPoseOverrideWeight = UKismetMathLibrary::Clamp(Value - GetCurveValue(FName("DisableLeftHandPoseOverride")), 0.0f, 1.0f);
}

void UFPSItemAnimLayerBase::SetBaseLastPivotTime(float InTime)
{
	// Safely check if the pointer exists before writing to it
	if (GetFPSAnimInstance())
	{
		// Call the thread-safe setter you already created on the Base Instance!
		GetFPSAnimInstance()->SetLastPivotTime(InTime);
	}
}