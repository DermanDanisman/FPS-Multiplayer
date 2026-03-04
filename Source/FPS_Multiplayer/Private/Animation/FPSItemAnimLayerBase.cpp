// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "Animation/FPSItemAnimLayerBase.h"

#include "AnimCharacterMovementLibrary.h"
#include "SequenceEvaluatorLibrary.h"
#include "Animation/FPSAnimInstanceBase.h"
#include "GameFramework/CharacterMovementComponent.h"


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
	
	UFPSAnimInstanceBase* AnimInstanceBase = Cast<UFPSAnimInstanceBase>(GetOwningComponent()->GetAnimInstance());
	if (!AnimInstanceBase) return;
	
	UCharacterMovementComponent* CharacterMovementComponent = Cast<UCharacterMovementComponent>(TryGetPawnOwner()->GetMovementComponent());
	if (!CharacterMovementComponent) return;
	
	CachedLastUpdateVelocity = CharacterMovementComponent->GetLastUpdateVelocity();
	
	CachedRootYawOffset = AnimInstanceBase->RootYawOffset;
	CachedTimeSinceFiredWeapon = AnimInstanceBase->TimeSinceFiredWeapon;
	CachedBrakingFriction = CharacterMovementComponent->BrakingFriction;
	CachedGroundFriction = CharacterMovementComponent->GroundFriction;	
	CachedBrakingFrictionFactor = CharacterMovementComponent->BrakingFrictionFactor;
	CachedBrakingDecelerationWalking = CharacterMovementComponent->BrakingDecelerationWalking;
	
	bCachedHasAcceleration = AnimInstanceBase->bHasAcceleration;
	bCachedUseSeparateBrakingFriction = CharacterMovementComponent->bUseSeparateBrakingFriction;
	bShouldEnableFootPlacement = AnimInstanceBase->GetCurveValue(FName("DisableLegIK")) <= 0.0f && AnimInstanceBase->bUseFootPlacement; 
	bCachedIsCrouching = AnimInstanceBase->bIsCrouching;
	bCachedGameplayTagIsADS = AnimInstanceBase->bGameplayTagIsADS;
	bCachedIsOnGround = AnimInstanceBase->bIsOnGround;
	bCachedIsFalling = AnimInstanceBase->bIsFalling;
	bCachedIsJumping = AnimInstanceBase->bIsJumping;
}

void UFPSItemAnimLayerBase::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	
	UpdateBlendWeightData(DeltaSeconds);
	UpdateJumpFallData(DeltaSeconds);
	UpdateSkeletalControlData();
	UpdatePredictedStopDistance();
}

void UFPSItemAnimLayerBase::UpdateBlendWeightData(float DeltaTime)
{
	if ((!RaiseWeaponAfterFiringDuration && bCachedIsCrouching) || ((!bCachedIsCrouching && bCachedGameplayTagIsADS) && bCachedIsOnGround))
	{
		HipFireUpperBodyOverrideWeight = 0.0f;
		AimOffsetBlendWeight = 0.0f;
	}
	else
	{
		if ((CachedTimeSinceFiredWeapon < RaiseWeaponAfterFiringDuration) || 
			(bCachedGameplayTagIsADS && (bCachedIsCrouching || !bCachedIsOnGround)) || 
			GetCurveValue(FName("applyHipfireOverridePose")) > 0.0f)
		{
			HipFireUpperBodyOverrideWeight = 1.0f;
			AimOffsetBlendWeight = 1.0f;
		}
		else
		{
			HipFireUpperBodyOverrideWeight = FMath::FInterpTo(HipFireUpperBodyOverrideWeight, 0.0f, DeltaTime, 1.0f);
			if (FMath::Abs(CachedRootYawOffset) < 10.0f && bCachedHasAcceleration)
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
	if (bCachedIsFalling)
	{
		TimeFalling = TimeFalling + DeltaTime;
	}
	else
	{
		if (bCachedIsJumping)
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
		CachedLastUpdateVelocity, 
		bCachedUseSeparateBrakingFriction,
		CachedBrakingFriction,
		CachedGroundFriction,
		CachedBrakingFrictionFactor,
		CachedBrakingDecelerationWalking
	);

	float NormalizedStopLocation = UKismetMathLibrary::VSizeXY(LocalPredictedStopDistance);
	PredictedStopDistance = NormalizedStopLocation;
}

void UFPSItemAnimLayerBase::SetLeftHandPoseOverrideWeight(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	float Value = bEnableLeftHandPoseOverride ? 1.0f : 0.0f;
	LeftHandPoseOverrideWeight = UKismetMathLibrary::Clamp(Value - GetCurveValue(FName("DisableLeftHandPoseOverride")), 0.0f, 1.0f);
}

UFPSAnimInstanceBase* UFPSItemAnimLayerBase::GetFPSAnimInstance() const
{
	return Cast<UFPSAnimInstanceBase>(GetOwningComponent()->GetAnimInstance());
}
