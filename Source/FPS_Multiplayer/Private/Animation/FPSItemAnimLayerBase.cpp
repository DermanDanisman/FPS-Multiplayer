// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "Animation/FPSItemAnimLayerBase.h"

#include "AnimCharacterMovementLibrary.h"
#include "SequenceEvaluatorLibrary.h"
#include "Animation/FPSAnimInstanceBase.h"
#include "GameFramework/CharacterMovementComponent.h"


void UFPSItemAnimLayerBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	// Cache the base instance ONCE when the layer is linked
	if (USkeletalMeshComponent* SkelMesh = GetOwningComponent())
	{
		BaseAnimInstance = Cast<UFPSAnimInstanceBase>(SkelMesh->GetAnimInstance());
	}
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
	if (!BaseAnimInstance) return;

	// Curves are thread-safe, so we evaluate them here
	bShouldEnableFootPlacement = BaseAnimInstance->GetCurveValue(FName("DisableLegIK")) <= 0.0f && BaseAnimInstance->bUseFootPlacement; 

	// Run your layer's specific math functions safely
	UpdateBlendWeightData(DeltaSeconds);
	UpdateJumpFallData(DeltaSeconds);
	UpdateSkeletalControlData();
	UpdatePredictedStopDistance();
}

void UFPSItemAnimLayerBase::UpdateBlendWeightData(float DeltaTime)
{
	if ((!RaiseWeaponAfterFiringDuration && BaseAnimInstance->bIsCrouching) || 
		   ((!BaseAnimInstance->bIsCrouching && BaseAnimInstance->bGameplayTagIsADS) && BaseAnimInstance->bIsOnGround))
	{
		HipFireUpperBodyOverrideWeight = 0.0f;
		AimOffsetBlendWeight = 0.0f;
	}
	else
	{
		if ((BaseAnimInstance->TimeSinceFiredWeapon < RaiseWeaponAfterFiringDuration) || 
		   (BaseAnimInstance->bGameplayTagIsADS && (BaseAnimInstance->bIsCrouching || !BaseAnimInstance->bIsOnGround)) || 
		   GetCurveValue(FName("applyHipfireOverridePose")) > 0.0f)
		{
			HipFireUpperBodyOverrideWeight = 1.0f;
			AimOffsetBlendWeight = 1.0f;
		}
		else
		{
			HipFireUpperBodyOverrideWeight = FMath::FInterpTo(HipFireUpperBodyOverrideWeight, 0.0f, DeltaTime, 1.0f);
          
			if (FMath::Abs(BaseAnimInstance->RootYawOffset) < 10.0f && BaseAnimInstance->bHasAcceleration)
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
	if (BaseAnimInstance->bIsFalling)
	{
		TimeFalling = TimeFalling + DeltaTime;
	}
	else
	{
		if (BaseAnimInstance->bIsJumping)
			TimeFalling = 0.0f;
	}
}

void UFPSItemAnimLayerBase::UpdateSkeletalControlData()
{
	float Substraction;
	if (bDisableHandIK)
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
		BaseAnimInstance->CachedLastUpdateVelocity, 
		BaseAnimInstance->bCachedUseSeparateBrakingFriction,
		BaseAnimInstance->CachedBrakingFriction,
		BaseAnimInstance->CachedGroundFriction,
		BaseAnimInstance->CachedBrakingFrictionFactor,
		BaseAnimInstance->CachedBrakingDecelerationWalking
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
	if (BaseAnimInstance)
	{
		// Call the thread-safe setter you already created on the Base Instance!
		BaseAnimInstance->SetLastPivotTime(InTime);
	}
}

/*
UFPSAnimInstanceBase* UFPSItemAnimLayerBase::GetFPSAnimInstance() const
{
	return Cast<UFPSAnimInstanceBase>(GetOwningComponent()->GetAnimInstance());
}
*/
