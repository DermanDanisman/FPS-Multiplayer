// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "Animation/FPSAnimInstanceBase.h"

#include "KismetAnimationLibrary.h"
#include "Character/FPSPlayerCharacter.h"
#include "Component/FPSCharacterMovementComponent.h"

void UFPSAnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UFPSAnimInstanceBase::NativeUninitializeAnimation()
{
	Super::NativeUninitializeAnimation();
}

void UFPSAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	AFPSPlayerCharacter* Character = Cast<AFPSPlayerCharacter>(TryGetPawnOwner());
    if (!Character) return;

    UFPSCharacterMovementComponent* Movement = Cast<UFPSCharacterMovementComponent>(Character->GetMovementComponent());
    if (!Movement) return;
    
    CachedActorLocation = Character->GetActorLocation();
    CachedActorRotation = Character->GetActorRotation();
	CachedActorForwardVector = Character->GetActorForwardVector();
	CachedActorRightVector = Character->GetActorRightVector();
	CachedActorUpVector = Character->GetActorUpVector();
    CachedVelocity = Character->GetVelocity();
	CachedLastUpdateVelocity = Movement->GetLastUpdateVelocity();
    CachedAcceleration = Movement->GetCurrentAcceleration();
	if (Character->IsLocallyControlled())
	{
		CachedControlRotation = Character->GetControlRotation();
	}
	else
	{
		// Smoothly interpolate the snappy network packets!
		FRotator TargetRot = Character->GetReplicatedControlRotation();
		CachedControlRotation = FMath::RInterpTo(CachedControlRotation, TargetRot, DeltaSeconds, 15.0f);
	}
	
    CachedGravityZ = Movement->GetGravityZ();
	CachedJumpZVelocity = Movement->JumpZVelocity;
    CachedMaxWalkSpeed = Movement->MaxWalkSpeed;
    CachedCurrentWalkSpeed = Movement->GetWalkSpeed();
    CachedBrakingFriction = Movement->BrakingFriction;
    CachedGroundFriction = Movement->GroundFriction;
    CachedBrakingFrictionFactor = Movement->BrakingFrictionFactor;
    CachedBrakingDecelerationWalking = Movement->BrakingDecelerationWalking;

    CachedMovementMode = Movement->MovementMode;
    bCachedIsMovingOnGround = Movement->IsMovingOnGround();
    bCachedIsCrouching = Movement->IsCrouching();
    bCachedOrientRotationToMovement = Movement->bOrientRotationToMovement;
    bCachedUseSeparateBrakingFriction = Movement->bUseSeparateBrakingFriction;

    LayerStates = Character->GetLayerStates();
    bIsLocallyControlled = Character->IsLocallyControlled();
}

void UFPSAnimInstanceBase::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	
	UpdateLocationData(DeltaSeconds);
	UpdateRotationData(DeltaSeconds);
	UpdateVelocityData();
	UpdateAccelerationData();
	UpdatePivotState(DeltaSeconds);
	UpdateWallDetectionHeuristic();
	UpdateCharacterStateData(DeltaSeconds);
	UpdateBlendWeightData(DeltaSeconds);
	UpdateRootYawOffset(DeltaSeconds);
	UpdateAimingData();
	UpdateJumpFallData();
	
	// Only run procedural 1st-person camera math if we are the local player looking through the camera!
	if (bIsLocallyControlled)
	{
		UpdateProceduralAlpha();
		UpdatePelvisWeight();
		UpdateSwayData(DeltaSeconds);
		UpdateLagPosition(DeltaSeconds);
	}
	
	// Evaluate the function here and store it in the simple boolean variable!
	bEnableControlRig = ShouldEnableControlRig();
	
	bIsFirstUpdate = false;
}

//
// --- IFPSAnimInterface Functions ---
//

void UFPSAnimInstanceBase::SetFPSMode(bool bInFPSMode)
{
	bIsFPSMode = bInFPSMode;
}

void UFPSAnimInstanceBase::SetUnarmed(bool bInUnarmed)
{
	bIsUnarmed = bInUnarmed;
}

void UFPSAnimInstanceBase::SetADS(bool bInADS)
{
	bGameplayTagIsADS = bInADS;
}

void UFPSAnimInstanceBase::SetADSUpper(bool bInADSUpper)
{
	bIsADSUpper = bInADSUpper;
}

void UFPSAnimInstanceBase::SetFPSPelvisWeight(float InPelvisWeight)
{
	FPSPelvisWeight = InPelvisWeight;
}

void UFPSAnimInstanceBase::UpdateLocationData(float DeltaTime)
{
	FVector VectorResult = CachedActorLocation - WorldLocation;
	DisplacementSinceLastUpdate = VectorResult.Size2D();
	
	WorldLocation = CachedActorLocation;
	
	DisplacementSpeed = UKismetMathLibrary::SafeDivide(DisplacementSinceLastUpdate, DeltaTime);
	
	// Ignore distance delta on first tick. We need at least two updates to get a delta.
	if (bIsFirstUpdate)
	{
		DisplacementSinceLastUpdate = 0.0f;
		DisplacementSpeed = 0.0f;
	}
}

void UFPSAnimInstanceBase::UpdateRotationData(float DeltaTime)
{
	YawDeltaSinceLastUpdate = UKismetMathLibrary::NormalizedDeltaRotator(CachedActorRotation, WorldRotation).Yaw;
	YawDeltaSpeed = UKismetMathLibrary::SafeDivide(YawDeltaSinceLastUpdate, DeltaTime);
	
	WorldRotation = CachedActorRotation;
	
	float MultipliedYawDeltaSpeed;
	if (bIsCrouching || bGameplayTagIsADS)
	{
		MultipliedYawDeltaSpeed = 0.025f;
	}
	else
	{
		MultipliedYawDeltaSpeed = 0.0375f;
	}
	
	AdditiveLeanAngle = YawDeltaSpeed * MultipliedYawDeltaSpeed;
	
	Pitch = UKismetMathLibrary::ComposeRotators(CachedControlRotation, FRotator(180.0f, 0.0f, 0.0f)).Pitch;
	
	PitchRotator = FRotator(0.0f, 0.0f, Pitch);
	
	if (bIsFirstUpdate)
	{
		// Ignore yaw delta on first tick. We need at least two updates to get a delta.
		YawDeltaSinceLastUpdate = 0.0f;
		AdditiveLeanAngle = 0.0f;
	}
}

void UFPSAnimInstanceBase::UpdateVelocityData()
{
	bool bWasMovingLastUpdate = !LocalVelocity2D.IsZero();
    
	WorldVelocity = CachedVelocity;
	FVector WorldVelocity2D = FVector(WorldVelocity.X, WorldVelocity.Y, 0.f);
    
	LocalVelocity2D = UKismetMathLibrary::LessLess_VectorRotator(WorldVelocity2D, WorldRotation);
	
	LocalVelocityDirectionAngle = UKismetAnimationLibrary::CalculateDirection(WorldVelocity2D, WorldRotation);
	
	LocalVelocityDirectionAngleWithOffset = FRotator::NormalizeAxis(LocalVelocityDirectionAngle - RootYawOffset);
	
	LocalVelocityDirection = SelectCardinalDirectionFromAngle(LocalVelocityDirectionAngleWithOffset, CardianalDirectionDeadZone, 
		LocalVelocityDirection, bWasMovingLastUpdate);
	
	LocalVelocityDirectionNoOffset = SelectCardinalDirectionFromAngle(LocalVelocityDirectionAngle, CardianalDirectionDeadZone, 
		LocalVelocityDirectionNoOffset, bWasMovingLastUpdate);
}

void UFPSAnimInstanceBase::UpdateAccelerationData()
{
	FVector WorldAcceleration2D = FVector(CachedAcceleration.X, CachedAcceleration.Y, 0.f);
	LocalAcceleration2D = UKismetMathLibrary::LessLess_VectorRotator(WorldAcceleration2D, WorldRotation);
	bHasAcceleration = !UKismetMathLibrary::NearlyEqual_FloatFloat(LocalAcceleration2D.SizeSquared2D(), 0.0f, 0.000001f);
	
	FVector NormalizedWorldAcceleration2D = UKismetMathLibrary::Normal(WorldAcceleration2D, 0.0001f);
	FVector VectorLerp = UKismetMathLibrary::VLerp(PivotDirection2D, NormalizedWorldAcceleration2D, 0.5f);
	PivotDirection2D = UKismetMathLibrary::Normal(VectorLerp, 0.0001f);
	
	CardinalDirectionFromAcceleration = GetOppositeCardinalDirection(SelectCardinalDirectionFromAngle(
		UKismetAnimationLibrary::CalculateDirection(PivotDirection2D, WorldRotation), 
		CardianalDirectionDeadZone, 
		ELocomotionCardinalDirection::LSD_Forward, 
		false
	));
	
}

void UFPSAnimInstanceBase::UpdateWallDetectionHeuristic()
{
	bool bLocalAccl = UKismetMathLibrary::VSizeXY(LocalAcceleration2D) > 0.1f;
	bool bLocalVel = UKismetMathLibrary::VSizeXY(LocalVelocity2D) < 200.0f;
	FVector LocalAcclNormalized = UKismetMathLibrary::Normal(LocalAcceleration2D);
	FVector LocalVelNormalized = UKismetMathLibrary::Normal(LocalVelocity2D);
	float DotProduct = FVector::DotProduct(LocalAcclNormalized, LocalVelNormalized);
	bIsRunningIntoWall = bLocalAccl && bLocalVel && UKismetMathLibrary::InRange_FloatFloat(DotProduct, -0.6f, 0.6f);
}

void UFPSAnimInstanceBase::UpdateCharacterStateData(float DeltaTime)
{
	// On ground state
	bIsOnGround = bCachedIsMovingOnGround;

	// Crouch state
	bool WasCrouchingLastUpdate = bIsCrouching;
	bIsCrouching = bCachedIsCrouching;
	bCrouchStateChange = bIsCrouching != WasCrouchingLastUpdate;
	
	// ADS state
	bADSStateChange = bGameplayTagIsADS != bADSLastUpdate;
	bADSLastUpdate = bGameplayTagIsADS;

	// Weapon fired state
	if (bGameplayTagIsFiring)
	{
		TimeSinceFiredWeapon = 0.0f;
	}
	else
	{
		TimeSinceFiredWeapon = TimeSinceFiredWeapon + DeltaTime;
	}
	
	/*bIsSprinting = (LayerStates.Gait == EGait::EG_Sprinting);
	bIsAiming = (LayerStates.AimState == EAimState::EAS_ADS || LayerStates.AimState == EAimState::EAS_Zoomed);*/

	// In Air State
	bIsJumping = false;
	bIsFalling = false;
	if (CachedMovementMode == EMovementMode::MOVE_Falling)
	{
		if (WorldVelocity.Z > 0.f) bIsJumping = true;
		else bIsFalling = true;
	}
}

void UFPSAnimInstanceBase::UpdateBlendWeightData(float DeltaTime)
{
	float InterpWeightData = FMath::FInterpTo(UpperbodyDynamicAdditiveWeight, 0.0f, DeltaTime, 6.0f);
	if (IsAnyMontagePlaying() && bIsOnGround)
	{
		UpperbodyDynamicAdditiveWeight = 1.0f;
	}
	else
	{
		UpperbodyDynamicAdditiveWeight = InterpWeightData;
	}
}

// TurnInPlace #2
// This function handles updating the yaw offset depending on the current state of the Pawn owner.
void UFPSAnimInstanceBase::UpdateRootYawOffset(float DeltaTime)
{
	// When the feet aren't moving (e.g. during Idle),
	// offset the root in the opposite direction to the Pawn owner's rotation to keep the mesh from rotating with the Pawn.
	if (RootYawOffsetMode == ERootYawOffsetMode::Accumulate)
	{
		SetRootYawOffset(RootYawOffset - YawDeltaSinceLastUpdate);
	}
	
	// When in motion, smoothly blend out the offset.
	if (bGameplayTagIsDashing || RootYawOffsetMode == ERootYawOffsetMode::BlendOut)
	{
		float SprintInterpResult = UKismetMathLibrary::FloatSpringInterp(
			RootYawOffset, 
			0.0, 
			RootYawOffsetSpringState, 
			80.0f, 
			1.0f, 
			DeltaTime, 
			1.0f, 
			0.5f
		);
		
		SetRootYawOffset(SprintInterpResult);
	}
	
	//Reset to blending out the yaw offset. Each update, a state needs to request to accumulate or hold the offset. 
	//Otherwise, the offset will blend out. 
	//This is primarily because the majority of states want the offset to blend out, so this saves on having to tag each state.
	RootYawOffsetMode = ERootYawOffsetMode::BlendOut;
}

void UFPSAnimInstanceBase::UpdateAimingData()
{
	AimPitch = UKismetMathLibrary::NormalizeAxis(CachedControlRotation.Pitch);
}

void UFPSAnimInstanceBase::UpdateJumpFallData()
{
	if (bIsJumping && CachedGravityZ != 0.0f)
	{
		TimeToJumpApex = (0.0f - WorldVelocity.Z) / CachedGravityZ;
	}
	else
	{
		TimeToJumpApex = 0.0f;
	}
}

void UFPSAnimInstanceBase::UpdatePivotState(float DeltaTime)
{
	// 1. Your original excellent logic to check the raw direction
	bool bPivotalInitialDirectionFB = PivotInitialDirection == ELocomotionCardinalDirection::LSD_Forward || PivotInitialDirection == ELocomotionCardinalDirection::LSD_Backward;
	bool bLocalVelocityDirectionFB = LocalVelocityDirection == ELocomotionCardinalDirection::LSD_Forward || LocalVelocityDirection == ELocomotionCardinalDirection::LSD_Backward;
	bool bPivotInitialDirectionLR = PivotInitialDirection == ELocomotionCardinalDirection::LSD_Left || PivotInitialDirection == ELocomotionCardinalDirection::LSD_Right;
	bool bLocalVelocityDirectionLR = LocalVelocityDirection == ELocomotionCardinalDirection::LSD_Left || LocalVelocityDirection == ELocomotionCardinalDirection::LSD_Right;
    
	bool bIsRawPerpendicular = (bPivotalInitialDirectionFB && !bLocalVelocityDirectionFB) || (bPivotInitialDirectionLR && !bLocalVelocityDirectionLR);

	// 2. The Grace Period Timer
	if (bIsRawPerpendicular)
	{
		PivotCancelTimer += DeltaTime; // Network might be stuttering, start the clock!
	}
	else
	{
		PivotCancelTimer = 0.0f; // Network stabilized and direction is correct, reset!
	}

	// 3. The Final Verdict
	bShouldCancelPivot = PivotCancelTimer >= PivotCancelTolerance;
}

void UFPSAnimInstanceBase::UpdateProceduralAlpha()
{
	if (bIsADSUpper)
	{
		SwayAlpha = 0.42f;
		LocationLagAlpha = 0.6f;
		RotationLagAlpha = 0.8f;
	}
	else
	{
		SwayAlpha = 1.0f;
		LocationLagAlpha = 1.0f;
		RotationLagAlpha = 1.0f;
	}
	
	if (bIsCrouching && !bGameplayTagIsADS)
	{
		CrouchAlpha = 1.0f;
	}
	else
	{
		CrouchAlpha = 0.0f;
	}

}

void UFPSAnimInstanceBase::UpdateSwayData(float DeltaTime)
{
	FRotator DeltaRotator = UKismetMathLibrary::NormalizedDeltaRotator(CachedControlRotation, CachedActorRotation);
	float NormalizedRange = UKismetMathLibrary::NormalizeToRange(DeltaRotator.Pitch, -90.f, 90.f);
	
	float YLerp = UKismetMathLibrary::Lerp(3.0f, -3.0f, NormalizedRange);
	float ZLerp = UKismetMathLibrary::Lerp(2.0f, -2.0f, NormalizedRange);
	
	PitchOffsetPosition = FVector(0.0f, YLerp, ZLerp);
	
	CurrentCameraRotation = CachedControlRotation;
	FRotator DeltaCameraRotator = UKismetMathLibrary::NormalizedDeltaRotator(CurrentCameraRotation, PreviousCameraRotation);
	float CameraClampY = UKismetMathLibrary::Clamp(DeltaCameraRotator.Pitch, -5.0f, 5.0f) * -1.f;
	float CameraClampZ = UKismetMathLibrary::Clamp(DeltaCameraRotator.Yaw, -5.0f, 5.0f);
	FRotator FinalCameraRotator = FRotator(0.0f, CameraClampZ * SwayAlpha, CameraClampY * SwayAlpha);
	CameraRotationRate = UKismetMathLibrary::RInterpTo(CameraRotationRate, FinalCameraRotator, DeltaTime, 5.0f /*(1.0f / DeltaTime) / 24.f*/);
	
	CameraRotationOffset = FVector(
		UKismetMathLibrary::Lerp(-10.f, 10.f, UKismetMathLibrary::NormalizeToRange(CameraRotationRate.Roll, -5.0f, 5.0f)) * 0.1f, 
		0.0f, 
		UKismetMathLibrary::Lerp(-6.f, 6.f, UKismetMathLibrary::NormalizeToRange(CameraRotationRate.Yaw, -5.0f, 5.0f)) * 0.1f
	);
	
	PreviousCameraRotation = CachedControlRotation;
}

void UFPSAnimInstanceBase::UpdateLagPosition(float DeltaTime)
{
	float VelocityDotForwardVector = UKismetMathLibrary::Dot_VectorVector(WorldVelocity, CachedActorForwardVector);
	float VelocityDotRightVector = UKismetMathLibrary::Dot_VectorVector(WorldVelocity, CachedActorRightVector);
	float VelocityDotUpVector = UKismetMathLibrary::Dot_VectorVector(WorldVelocity, CachedActorUpVector);
	
	float Divide_1 = UKismetMathLibrary::SafeDivide(VelocityDotForwardVector, CachedMaxWalkSpeed) * -1.0f;
	float Divide_2 = UKismetMathLibrary::SafeDivide(VelocityDotRightVector, CachedMaxWalkSpeed);
	float Divide_3 = UKismetMathLibrary::SafeDivide(VelocityDotUpVector, CachedJumpZVelocity) * -1.0f;		
	
	FVector TempLagPosition = FVector(Divide_2 * 0.3f, Divide_1 * 0.4f, Divide_3);
	FVector ClampedTempLagPosition = UKismetMathLibrary::ClampVectorSize(TempLagPosition * 2.0f, 0.0f, 4.0f);
	
	LocationLagPosition = UKismetMathLibrary::VInterpTo(LocationLagPosition, ClampedTempLagPosition, DeltaTime, 6.0f /*(1.0f / DeltaTime) / 6.0f*/);
	
	LocationLagPosition = FVector(LocationLagPosition.X * RotationLagAlpha, LocationLagPosition.Y * LocationLagAlpha, LocationLagPosition.Z);
	
	FRotator AirTiltPitch = FRotator((LocationLagPosition.Z * -2.0f), 0.0f, 0.0f);
	AirTiltRotator = UKismetMathLibrary::RInterpTo(AirTiltRotator, AirTiltPitch, DeltaTime, 12.0f /*(1.0f / DeltaTime) / 12.0f*/);
	FVector AirOffsetX = FVector(LocationLagPosition.Z * -0.5f, 0.0f, 0.0f);
	AirOffset = UKismetMathLibrary::VInterpTo(AirOffset, AirOffsetX, DeltaTime, 12.0f /*(1.0f / DeltaTime) / 12.0f*/);
}

void UFPSAnimInstanceBase::UpdatePelvisWeight()
{
	if (bIsFPSMode)
	{
		ApplyPelvisWeight = 1.0f - FPSPelvisWeight;
	}
	else
	{
		ApplyPelvisWeight = 0.0f;
	}
}

//
// --- Turn In Place Functions ---
//

void UFPSAnimInstanceBase::SetRootYawOffset(float NewRootYawOffset)
{
	if (bEnableRootYawOffset)
	{
		// TurnInPlace #3
		// We clamp the offset because at large offsets the character has to aim too far backwards, which over twists the spine. 
		// The turn in place animations will usually keep up with the offset, but this clamp will cause the feet to slide if the user rotates the camera too quickly.
		// If desired, this clamp could be replaced by having aim animations that can go up to 180 degrees or by triggering turn in place animations more aggressively.
		FVector2D FinalClamp;
		if (bIsCrouching)
		{
			FinalClamp = RootYawOffsetAngleClampCrouched;
		}
		else
		{
			FinalClamp = RootYawOffsetAngleClamp;
		}
		
		float NormalizedNewRootYawOffset = UKismetMathLibrary::NormalizeAxis(NewRootYawOffset);
		if (FinalClamp.X != FinalClamp.Y)
		{
			RootYawOffset = UKismetMathLibrary::ClampAngle(NormalizedNewRootYawOffset, FinalClamp.X, FinalClamp.Y);;
		}
		else
		{
			RootYawOffset = NormalizedNewRootYawOffset;
		}
		
		// TurnInPlace #4
		// We want aiming to counter the yaw offset to keep the weapon aiming in line with the camera.
		AimYaw = RootYawOffset * -1.f;
	}
	else
	{
		RootYawOffset = 0.0f;
		AimYaw = 0.0f;
	}
}

/**
 * TurnInPlace #5
 * When the yaw offset gets too big, we trigger TurnInPlace animations to rotate the character back. 
 * E.g. if the camera is rotated 90 degrees to the right, it will be facing the character's right shoulder. 
 * If we play an animation that rotates the character 90 degrees to the left, the character will once again be facing away from the camera.
 * We use the "TurnYawAnimModifier" animation modifier to generate the necessary curves on each TurnInPlace animation.
 * See ABP_ItemAnimLayersBase for examples of triggering TurnInPlace animations.
 */
void UFPSAnimInstanceBase::ProcessTurnYawCurve()
{
	// The "TurnYawWeight" curve is set to 1 in TurnInPlace animations, so its current value from GetCurveValue will be the current weight of the TurnInPlace animation. 
	// We can use this to "unweight" the TurnInPlace animation to get the full RemainingTurnYaw curve value. 
	
	float PreviousTurnYawCurveValue = TurnYawCurveValue;
	float TurnYawWeightCurveValue = GetCurveValue(FName("TurnYawWeight"));
	float RemainingTurnYawCurveValue = GetCurveValue(FName("RemainingTurnYaw"));
	if (UKismetMathLibrary::NearlyEqual_FloatFloat(TurnYawWeightCurveValue, 0.0f, 0.0001f))
	{
		TurnYawCurveValue = 0.0f;
		PreviousTurnYawCurveValue = 0.0f;
	}
	else
	{
		TurnYawCurveValue = RemainingTurnYawCurveValue / TurnYawWeightCurveValue;
	}
	
	// Avoid applying the curve delta when the curve first becomes relevant.
	// E.g. When a turn animation starts, the previous curve value will be 0 and the current value will be 90, but no actual rotation has happened yet.
	if (PreviousTurnYawCurveValue != 0.0f)
	{
		float DeltaTurnYaw = TurnYawCurveValue - PreviousTurnYawCurveValue;
		SetRootYawOffset(RootYawOffset - DeltaTurnYaw);
	}
}

//
// --- Helper Functions ---
//

ELocomotionCardinalDirection UFPSAnimInstanceBase::SelectCardinalDirectionFromAngle(float NewAngle,
                                                                                    float NewDeadZone, ELocomotionCardinalDirection CurrentDirection, bool bUseCurrentDirection) const
{
	float AbsoluteNewAngle = FMath::Abs(NewAngle);
	float ForwardDeadZone = NewDeadZone;
	float BackwardDeadZone = NewDeadZone;

	if (bUseCurrentDirection)
	{
		switch (CurrentDirection)
		{
		case ELocomotionCardinalDirection::LSD_Forward:
			ForwardDeadZone *= 2.0f;
			break;
		case ELocomotionCardinalDirection::LSD_Backward:
			BackwardDeadZone *= 2.0f;
			break;
		default:
			break;
		}
	}

	if (AbsoluteNewAngle <= (45.f + ForwardDeadZone)) return ELocomotionCardinalDirection::LSD_Forward;
	if (AbsoluteNewAngle >= (135.f - BackwardDeadZone)) return ELocomotionCardinalDirection::LSD_Backward;
	if (NewAngle > 0.0f) return ELocomotionCardinalDirection::LSD_Right;

	return ELocomotionCardinalDirection::LSD_Left;
}

ELocomotionCardinalDirection UFPSAnimInstanceBase::GetOppositeCardinalDirection(
	ELocomotionCardinalDirection CurrentDirection) const
{
	switch (CurrentDirection)
	{
	case ELocomotionCardinalDirection::LSD_Forward:
		return ELocomotionCardinalDirection::LSD_Backward;
	case ELocomotionCardinalDirection::LSD_Backward:
		return ELocomotionCardinalDirection::LSD_Forward;
	case ELocomotionCardinalDirection::LSD_Left:
		return ELocomotionCardinalDirection::LSD_Right;
	case ELocomotionCardinalDirection::LSD_Right:
		return ELocomotionCardinalDirection::LSD_Left;
	case ELocomotionCardinalDirection::LSD_MAX:
		return ELocomotionCardinalDirection::LSD_Forward;
	}
	
	return ELocomotionCardinalDirection::LSD_Forward;
}

bool UFPSAnimInstanceBase::ShouldEnableControlRig() const
{
	float CurveValue = GetCurveValue(FName("DisableLegIK"));
	return !bUseFootPlacement && CurveValue <= 0.0f;
}

/*bool UFPSAnimInstanceBase::IsMovingPerpendicularToInitialPivot()
{
	// We stay in a pivot when pivoting along a line (e.g. triggering a left-right pivot while playing a right-left pivot), but break out if the character makes a perpendicular change in direction.
	
	bool bPivotalInitialDirectionFB = PivotInitialDirection == ELocomotionCardinalDirection::LSD_Forward || PivotInitialDirection == ELocomotionCardinalDirection::LSD_Backward;
	bool bLocalVelocityDirectionFB = LocalVelocityDirection == ELocomotionCardinalDirection::LSD_Forward || LocalVelocityDirection == ELocomotionCardinalDirection::LSD_Backward;
	bool bPivotInitialDirectionLR = PivotInitialDirection == ELocomotionCardinalDirection::LSD_Left || PivotInitialDirection == ELocomotionCardinalDirection::LSD_Right;
	bool bLocalVelocityDirectionLR = LocalVelocityDirection == ELocomotionCardinalDirection::LSD_Left || LocalVelocityDirection == ELocomotionCardinalDirection::LSD_Right;
	
	return (bPivotalInitialDirectionFB && !bLocalVelocityDirectionFB) || (bPivotInitialDirectionLR && !bLocalVelocityDirectionLR);
}*/