// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "FPS_Multiplayer/Public/Animation/FPSAnimInstance.h"

#include "AnimCharacterMovementLibrary.h"
#include "TurnInPlace.h"
#include "TurnInPlaceStatics.h"
#include "Actor/Weapon/FPSWeapon.h"
#include "Camera/CameraComponent.h"
#include "Component/FPSCharacterMovementComponent.h"
#include "Component/FPSCombatComponent.h"
#include "FPS_Multiplayer/Public/Character/FPSPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

#pragma region Lifecycle

// =========================================================================
//                           LIFECYCLE
// =========================================================================

void UFPSAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    UpdateReferences();
	
	// Set the initial world location before the first frame runs
	if (AActor* OwningActor = GetOwningActor())
	{
		WorldLocation = OwningActor->GetActorLocation();
	}
}

void UFPSAnimInstance::UpdateReferences()
{
    if (AFPSPlayerCharacter* Character = GetFPSCharacter())
    {
       // 1. SAFETY CHECK: Ensure Combat Component exists
       UFPSCombatComponent* Combat = Character->GetCombatComponent();
       if (Combat)
       {
          // 2. SAFETY UNBIND: Prevent duplicate bindings or stale references
          Combat->OnWeaponEquipped.Remove(OnWeaponEquippedDelegateHandle);
            
          // 3. BIND: Listen for weapon changes
          OnWeaponEquippedDelegateHandle = Combat->OnWeaponEquipped.AddUObject(this, &ThisClass::OnCharacterWeaponEquipped);
          
          // 4. INITIAL SYNC: If we spawned late and weapon is ALREADY there
          if (Combat->GetEquippedWeapon())
          {
              OnCharacterWeaponEquipped(Combat->GetEquippedWeapon());
          }
       }
    }
}

void UFPSAnimInstance::NativeUninitializeAnimation()
{
    if (GetFPSCharacter())
    {
       if (UFPSCombatComponent* Combat = GetFPSCharacter()->GetCombatComponent())
       {
          Combat->OnWeaponEquipped.Remove(OnWeaponEquippedDelegateHandle);
       }
    }
    
    Super::NativeUninitializeAnimation();
}

// Runs every frame on the Game Thread.
// ARCHITECTURE NOTE: We do NOT do heavy logic here. We simply copy data
// from the Character to local variables so the AnimGraph (Worker Thread) can read them safely.
void UFPSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    
    if (!GetFPSCharacter() || !GetMovementComponent()) return;
    	
	UpdateLocationData(DeltaSeconds);
	UpdateRotationData();
	UpdateVelocityData(DeltaSeconds);
	UpdateAccelerationData();
	UpdateCharacterState();
	UpdateAimingData();
	UpdateJumpFallData();
	UpdateWallDetectionHeuristic();
	
	// Abort out of start anim state if we exceed min turn angle while strafing
	bIsStrafing = !GetMovementComponent()->bOrientRotationToMovement;
	
	// Retrieve Turn in Place data from the game thread. This data is then processed in the function BlueprintThreadSafeUpdateAnimation.
	UTurnInPlaceStatics::UpdateTurnInPlace(
		GetFPSCharacter()->TurnInPlace, 
		DeltaSeconds, 
		TurnInPlaceAnimGraphData, 
		bIsStrafing, 
		TurnInPlaceAnimGraphOutput, 
		bCanUpdateTurnInPlace
	);
    
    // Calculate IK / Transforms
    CalculateHandTransforms();     // Procedural ADS
    CalculateLeftHandTransform();  // Left Hand IK
	
	// We cache curve values in worker threads at the same point where the anim state is determined
	// The game thread can then request these values. 
	// If it queried the curve values at a different time resulting in a different state then the anim state will become unreliable
	TurnInPlaceCurveValues = UTurnInPlaceStatics::ThreadSafeUpdateTurnInPlaceCurveValues(this, TurnInPlaceAnimGraphData);
	
	// Process Turn In Place Data that was retrieved from the game thread in BlueprintUpdateAnimation
	UTurnInPlaceStatics::ThreadSafeUpdateTurnInPlace(TurnInPlaceAnimGraphData, bCanUpdateTurnInPlace, bIsStrafing, TurnInPlaceAnimGraphOutput);
}

#pragma endregion Lifecycle

#pragma region Data Gathering

// =========================================================================
//                           CALCULATIONS: DATA
// =========================================================================

void UFPSAnimInstance::UpdateLocationData(float DeltaTime)
{
	AActor* OwningActor = GetOwningActor();
	if (!OwningActor) return; // Safety check

	FVector CurrentLocation = OwningActor->GetActorLocation();
    
	// Because WorldLocation was initialized properly, this is naturally 0.f on frame 1
	DisplacementSinceLastUpdate = UKismetMathLibrary::VSizeXY(CurrentLocation - WorldLocation);
	WorldLocation = CurrentLocation;
    
	DisplacementSpeed = UKismetMathLibrary::SafeDivide(DisplacementSinceLastUpdate, DeltaTime);
	
	Speed3D = WorldVelocity.Size();
	GroundSpeed = WorldVelocity2D.Size(); // Ignore Z for locomotion
	
}

void UFPSAnimInstance::UpdateRotationData()
{
	// =========================================================
	// UPDATE AIM OFFSETS FOR TURN IN PLACE
	// =========================================================

	// 2. GET PITCH (Up/Down)
	// We calculate this standard way (Control Rot - Actor Rot)
	FRotator ControlRot = GetFPSCharacter()->GetReplicatedControlRotation();
	FRotator ActorRot = GetFPSCharacter()->GetActorRotation();
	FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(ControlRot, ActorRot);
	AimPitch = Delta.Pitch;
	RootYawOffset = Delta.Yaw;
}

void UFPSAnimInstance::UpdateVelocityData(float DeltaTime)
{
	AActor* OwningActor = GetOwningActor();
	if (!OwningActor) return; 
    
	bool bWasMovingLastUpdate = !LocalVelocity2D.IsNearlyZero(0.000001f);
    
	WorldVelocity = OwningActor->GetVelocity();
	WorldVelocity2D = FVector(WorldVelocity.X, WorldVelocity.Y, 0.f);
    
	// Pure C++ way to get Local Velocity (Replaces the Kismet 'LessLess' node)
	LocalVelocity2D = OwningActor->GetActorRotation().UnrotateVector(WorldVelocity2D);
    
	bHasVelocity = !LocalVelocity2D.IsNearlyZero(0.000001f);
    
	// --- FIX 2: Bulletproof Angle Math ---
	// Only calculate a new angle if speed > 5 cm/s (SizeSquared > 25.0f). 
	// This prevents erratic angle snapping when velocity crosses through 0.0 during quick WASD changes.
	if (WorldVelocity2D.SizeSquared() > 25.0f) 
	{
		float VelocityYaw = WorldVelocity2D.ToOrientationRotator().Yaw;
		LocalVelocityDirectionAngle = FRotator::NormalizeAxis(VelocityYaw - OwningActor->GetActorRotation().Yaw);
	}
    
	// Always calculate the Offset angle so Orientation Warping perfectly counters the Turn-In-Place twisting
	LocalVelocityDirectionAngleWithOffset = FRotator::NormalizeAxis(LocalVelocityDirectionAngle - RootYawOffset);
    
	// --- SMOOTHING FOR ORIENTATION WARPING ---
	FRotator CurrentRot(0.f, SmoothedLocomotionAngle, 0.f);
	FRotator TargetRot(0.f, LocalVelocityDirectionAngleWithOffset, 0.f);
    
	CurrentRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, LocomotionAngleInterpSpeed);
	SmoothedLocomotionAngle = CurrentRot.Yaw;
    
	// Update Cardinal Directions
	LocalVelocityDirection = SelectCardinalDirectionFromAngle(LocalVelocityDirectionAngleWithOffset, CardinalDirectionDeadZone, LocalVelocityDirection, bWasMovingLastUpdate);
	LocalVelocityDirectionNoOffset = SelectCardinalDirectionFromAngle(LocalVelocityDirectionAngle, CardinalDirectionDeadZone, LocalVelocityDirectionNoOffset, bWasMovingLastUpdate);
}

void UFPSAnimInstance::UpdateAccelerationData()
{
	// Acceleration is replicated via the GetMovementComponent(). Input is not.
	WorldAcceleration = GetMovementComponent()->GetCurrentAcceleration();
	WorldAcceleration2D = FVector(WorldAcceleration.X, WorldAcceleration.Y, 0.f);
	LocalAcceleration2D = UKismetMathLibrary::LessLess_VectorRotator(WorldAcceleration2D, GetOwningActor()->GetActorRotation());
	bHasAcceleration = !UKismetMathLibrary::NearlyEqual_FloatFloat(LocalAcceleration2D.SizeSquared(), 0.f, 0.000001f);
}

void UFPSAnimInstance::UpdateCharacterState()
{
	// On Ground State
	bIsOnGround = GetMovementComponent()->IsMovingOnGround();
	
	// Crouch State
	bool WasCrouchingLastUpdate = bIsCrouching;
	bIsCrouching = GetMovementComponent()->IsCrouching();
	bCrouchStateChanged = bIsCrouching != WasCrouchingLastUpdate;
	
	// Sprinting State
	bIsSprinting = (LayerStates.Gait == EGait::EG_Sprinting);
	
	// ADS State
	bIsAiming = (LayerStates.AimState == EAimState::EAS_ADS || LayerStates.AimState == EAimState::EAS_Zoomed);
	
	// In Air State
	bIsJumping = false;
	bIsFalling = false;
	if (GetMovementComponent()->MovementMode == EMovementMode::MOVE_Falling)
	{
		if (WorldVelocity.Z > 0.f)
			bIsJumping = true;
		else
			bIsFalling = true;
	}
	
	// "ShouldMove" gate: Only true if we have input AND reasonable speed.
	// Threshold (3.0f) prevents tiny micro-movements (analog drift) from triggering start animations.
	bShouldMove = bHasAcceleration && (GroundSpeed > 3.0f);
	
	// --- SYNC LAYER STATES ---
	// The AnimInstance simply copies the Authoritative State from the Character.
	LayerStates = GetFPSCharacter()->GetLayerStates();
	bIsLocallyControlled = GetFPSCharacter()->IsLocallyControlled();
}

void UFPSAnimInstance::UpdateAimingData()
{
	if (!GetFPSCharacter()) return;
	if (!GetFPSCharacter()->GetCombatComponent()) return;
	if (!GetFPSCharacter()->GetCombatComponent()->GetEquippedWeapon()) return;
	
	// 1. Get the Rotations
	FRotator ControlRotation = GetFPSCharacter()->GetReplicatedControlRotation();
	FRotator ActorRotation = GetFPSCharacter()->GetActorRotation();
    
	// 2. Find the Local Aim Offset
	// This gives us a clean -90 to 90 Pitch value, completely ignoring world compass direction!
	FRotator AimOffset = UKismetMathLibrary::NormalizedDeltaRotator(ControlRotation, ActorRotation);
    
	// 3. Divide by 8 bones and invert. 
	// Example: Looking up 80 deg -> Each bone rotates -10 deg.
	float PitchPerBone = (AimOffset.Pitch / 8.0f) * -1.0f; 
    
	// 4. Apply to the correct axis (Assuming Roll for Spine Bones in this skeleton)
	FRotator TargetPitchPerBone = FRotator(0.f, 0.f, PitchPerBone);
    
	// 5. INTERPOLATE (Smoothing)
	PitchValuePerBone = FMath::RInterpTo(
	   PitchValuePerBone,      
	   TargetPitchPerBone,     
	   GetDeltaSeconds(),      
	   PitchPerBoneInterpSpeed
	);
}

void UFPSAnimInstance::UpdateJumpFallData()
{
	if (bIsJumping)
	{
		TimeToJumpApex = (0.0f - WorldVelocity.Z) / TryGetPawnOwner()->GetMovementComponent()->GetGravityZ();
	}
	else
	{
		TimeToJumpApex = 0.0f;
	}
}

// This logic guesses if the character is running into a wall by checking if there's a large angle between velocity and acceleration 
// (i.e. the character is pushing towards the wall but actually sliding to the side) and if the character is trying to accelerate but speed is relatively low.
void UFPSAnimInstance::UpdateWallDetectionHeuristic()
{
	bool bLocalAccl = UKismetMathLibrary::VSizeXY(LocalAcceleration2D) > 0.1f;
	bool bLocalVel = UKismetMathLibrary::VSizeXY(LocalVelocity2D) < 200.0f;
	FVector LocalAcclNormalized = UKismetMathLibrary::Normal(LocalAcceleration2D);
	FVector LocalVelNormalized = UKismetMathLibrary::Normal(LocalVelocity2D);
	float DotProduct = FVector::DotProduct(LocalAcclNormalized, LocalVelNormalized);
	bIsRunningIntoWall = bLocalAccl && bLocalVel && UKismetMathLibrary::InRange_FloatFloat(DotProduct, -0.6f, 0.6f);
	
}

#pragma endregion Data Gathering

#pragma region Math & Physics

// =========================================================================
//                        CALCULATIONS: MATH & PHYSICS
// =========================================================================

void UFPSAnimInstance::CalculateLeftHandTransform()
{
    // 1. Safety Checks
    if (!GetFPSCharacter() || !GetFPSCharacter()->GetCombatComponent()) return;
    
    AFPSWeapon* EquippedWeapon = GetFPSCharacter()->GetCombatComponent()->GetEquippedWeapon();
    if (!EquippedWeapon || !EquippedWeapon->GetWeaponMesh()) return;

    // 2. Get the Socket Transform in World Space
    FTransform SocketTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), RTS_World);
    
    // 3. Transform World Space -> Bone Space (Relative to RIGHT HAND)
    FVector OutPosition;
    FRotator OutRotation;
    
    GetFPSCharacter()->GetMesh()->TransformToBoneSpace(
       HandBoneName, 
       SocketTransform.GetLocation(), 
       SocketTransform.Rotator(), // Use socket's actual rotation for grip alignment
       OutPosition, 
       OutRotation
    );
    
    // 4. Set the Variable
    LeftHandTargetTransform.SetLocation(OutPosition);
    LeftHandTargetTransform.SetRotation(FQuat(OutRotation));
}

#pragma endregion Math & Physics

#pragma region Procedural Aiming

// =========================================================================
//                        PROCEDURAL AIMING (ADS)
// =========================================================================

void UFPSAnimInstance::CalculateHandTransforms()
{
    // =========================================================
    // 1. SAFETY & NETWORK OPTIMIZATION
    // =========================================================
    
    if (!GetFPSCharacter()) return;
    
    // [OPTIMIZATION] IsLocallyControlled Check
    // We strictly ONLY run this expensive math for the player controlling this character.
    // Simulated Proxies (other players) do not need perfect sight alignment.
    if (!GetFPSCharacter()->IsLocallyControlled()) 
    {
        HandLocation = FVector::ZeroVector;
        HandRotation = FRotator::ZeroRotator;
        return;
    }

    // =========================================================
    // 2. PREPARE WORLD DATA
    // =========================================================
    
    FTransform HandWorldTransform = GetFPSCharacter()->GetMesh()->GetSocketTransform(HandBoneName, RTS_World);
    FTransform HeadWorldTransform = GetFPSCharacter()->GetMesh()->GetSocketTransform(HeadBoneName, RTS_World);
    FTransform CameraWorldTransform;
    
    // [NETWORK FIX] The "Proxy Camera"
    if (GetFPSCharacter()->IsLocallyControlled())
    {
       // Local Player: Use the actual Camera Component
       CameraWorldTransform = GetFPSCharacter()->GetCameraComponent()->GetComponentTransform();
    }
    else
    {
       // Remote Player: They don't have a camera at their eyes. We simulate one.
       // Use Network Pitch + Body Yaw
       float NetworkPitch = GetFPSCharacter()->GetReplicatedControlRotation().Pitch;
       float VisualYaw = HeadWorldTransform.Rotator().Yaw;
       
       FRotator FakeCameraRot = FRotator(NetworkPitch, VisualYaw, 0.f);
       CameraWorldTransform = FTransform(FakeCameraRot, HeadWorldTransform.GetLocation(), FVector::OneVector);
    }

    // =========================================================
    // 3. ITERATE CACHED SIGHTS
    // =========================================================
    
    // We iterate the pre-cached list (built on Equip) to avoid slow "GetComponents" calls.
    for (const FCachedSightData& Sight : CachedSights)
    {
        if (!Sight.Mesh.IsValid()) continue;

        FTransform FinalTransform = FTransform::Identity;

        // --- LOGIC A: OPTIC SIGHTS (Red Dots / Scopes) ---
        // Logic: Move Hand so that Sight Socket aligns with Camera Center.
        if (Sight.bIsOptic)
        {
            FTransform SightTransform = Sight.Mesh->GetSocketTransform(Sight.SocketNameA, RTS_World);
            FTransform HandToSight = UKismetMathLibrary::MakeRelativeTransform(HandWorldTransform, SightTransform);

            FVector AdjLoc = HandToSight.GetLocation();
            AdjLoc.X += CurrentDistanceFromCamera; // Push forward from camera
            
            FTransform AdjTrans(HandToSight.GetRotation(), AdjLoc, FVector::OneVector);
            FTransform NewHandWorldTransform = AdjTrans * CameraWorldTransform;
            
            // Convert back to Head Space for AnimGraph
            FinalTransform = UKismetMathLibrary::MakeRelativeTransform(NewHandWorldTransform, HeadWorldTransform);
        }
        // --- LOGIC B: IRON SIGHTS (Vector Alignment) ---
        // Logic: Align Front Post and Rear Notch vector to the Camera vector.
        else
        {
            FVector FrontLocation = Sight.Mesh->GetSocketLocation(Sight.SocketNameA);
            
            // Handle split meshes (e.g. Receiver vs Barrel)
            UMeshComponent* RearMeshToUse = Sight.RearMesh.IsValid() ? Sight.RearMesh.Get() : Sight.Mesh.Get();
            if (!RearMeshToUse) continue;
            
            FTransform RearTransform = Sight.Mesh->GetSocketTransform(Sight.SocketNameB, RTS_World);
            FVector RearLocation = RearTransform.GetLocation();
            FRotator RearRotation = RearTransform.Rotator();

            // Calculate the geometric alignment needed
            FVector DirectionToTarget = (FrontLocation - RearLocation).GetSafeNormal();
            FVector UpVector = UKismetMathLibrary::GetUpVector(RearRotation);
            FVector RightVector = UKismetMathLibrary::GetRightVector(RearRotation);

            float UpDot = FVector::DotProduct(DirectionToTarget, UpVector);
            float RightDot = FVector::DotProduct(DirectionToTarget, RightVector);
            
            float AnglePitch = UKismetMathLibrary::DegAsin(UpDot);
            float AngleYaw = UKismetMathLibrary::DegAsin(RightDot);

            FRotator PitchRot = UKismetMathLibrary::RotatorFromAxisAndAngle(RightVector, -AnglePitch); 
            FRotator YawRot = UKismetMathLibrary::RotatorFromAxisAndAngle(UpVector, AngleYaw);

            FRotator FinalSightRotation = UKismetMathLibrary::ComposeRotators(
                UKismetMathLibrary::ComposeRotators(PitchRot, YawRot), RearRotation);

            FTransform TrueSightTransform(FinalSightRotation, RearLocation, FVector::OneVector);
            FTransform HandToSight = UKismetMathLibrary::MakeRelativeTransform(HandWorldTransform, TrueSightTransform);

            FVector AdjLoc = HandToSight.GetLocation();
            AdjLoc.X = CurrentDistanceFromCamera;
            
            FTransform AdjTrans(HandToSight.GetRotation(), AdjLoc, FVector::OneVector);
            FTransform NewHandWorld = AdjTrans * CameraWorldTransform;
            FinalTransform = UKismetMathLibrary::MakeRelativeTransform(NewHandWorld, HeadWorldTransform);
        }

        // Store result
        if (HandTransforms.IsValidIndex(Sight.SightIndex))
        {
            HandTransforms[Sight.SightIndex] = FinalTransform;
        }
    }

    // =========================================================
    // 4. DETERMINE TARGET & SMOOTH
    // =========================================================
    
    FTransform DesiredTarget = FTransform::Identity;

    if (bIsAiming) // ADS
    {
        if (HandTransforms.IsValidIndex(CurrentSightIndex))
            DesiredTarget = HandTransforms[CurrentSightIndex];
    }
    else // HIP FIRE
    {
        // Use cached hip fire offset
        FTransform HipTargetWorldTransform = CurrentHipFireOffset * CameraWorldTransform;
        DesiredTarget = UKismetMathLibrary::MakeRelativeTransform(HipTargetWorldTransform, HeadWorldTransform);
    }
    
    // Smooth Transition (Interp)
    CurrentHandTransform = UKismetMathLibrary::TInterpTo(
       CurrentHandTransform, 
       DesiredTarget, 
       GetDeltaSeconds(), 
       AimInterpSpeed
    );

    HandLocation = CurrentHandTransform.GetLocation();
    HandRotation = CurrentHandTransform.Rotator();
}

#pragma endregion Procedural Aiming

#pragma region Events & Start

// =========================================================================
//                           EVENTS & UTILITIES
// =========================================================================

ELocomotionCardinalDirection UFPSAnimInstance::SelectCardinalDirectionFromAngle(float NewAngle, float NewDeadZone,
	ELocomotionCardinalDirection CurrentDirection, bool bUseCurrentDirection) const
{
	float AbsoluteNewAngle = UKismetMathLibrary::Abs(NewAngle);
	float ForwardDeadZone = NewDeadZone;
	float BackwardDeadZone = NewDeadZone;
	
	if (bUseCurrentDirection)
	{
		// If moving Fwd, double the Fwd dead zone.
		// It should be harder to leave Fwd when moving Fwd.
		// When moving Left/Right, the dead zone will be smaller so we don't rapidly toggle between directions.
		switch (CurrentDirection) 
		{
		case ELocomotionCardinalDirection::LSD_Forward:
			ForwardDeadZone *= 2.0f; // Cleaner syntax
			break;
		case ELocomotionCardinalDirection::LSD_Backward:
			BackwardDeadZone *= 2.0f; 
			break;
		default: 
			break;
		}
	}
	
	if (AbsoluteNewAngle <= (45.f + ForwardDeadZone))
	{
		return ELocomotionCardinalDirection::LSD_Forward;
	}
	if (AbsoluteNewAngle >= (135.f - BackwardDeadZone))
	{
		return ELocomotionCardinalDirection::LSD_Backward;
	}
	if (NewAngle > 0.0f)
	{
		return ELocomotionCardinalDirection::LSD_Right;
	}
	
	return ELocomotionCardinalDirection::LSD_Left;
}

void UFPSAnimInstance::OnCharacterWeaponEquipped(AFPSWeapon* NewWeapon)
{
    if (!NewWeapon) 
    {
       CurrentHipFireOffset = FTransform::Identity;
       HandTransforms.Empty();
       CachedSights.Empty();
       return; 
    }
    
    bIsArmed = IsValid(NewWeapon);

    CurrentHipFireOffset = NewWeapon->GetHipFireOffset();
    CurrentDistanceFromCamera = NewWeapon->GetDistanceFromCamera();
    CurrentRightHandEffectorLocation = NewWeapon->GetRightHandEffectorLocation();
    CurrentRightHandJointTargetLocation = NewWeapon->GetRightHandJointTargetLocation();
    TimeToAim = NewWeapon->GetTimeToAim();
    TimeFromAim = NewWeapon->GetTimeFromAim();

    // 2. Build Sight Cache (Optimized Tag Parsing)
    CachedSights.Empty();
    TArray<UMeshComponent*> WeaponMeshes;
    NewWeapon->GetComponents<UMeshComponent>(WeaponMeshes);

    const FName OpticSocket = NewWeapon->GetOpticSocketName();
    const FName FrontSocket = NewWeapon->GetFrontSightSocketName();
    const FName RearSocket  = NewWeapon->GetRearSightSocketName();
    
    const FString OpticPrefix = NewWeapon->GetOpticTagPrefix();
    const FString FrontPrefix = NewWeapon->GetFrontSightTagPrefix();
    const FString RearPrefix  = NewWeapon->GetRearSightTagPrefix();

    for (UMeshComponent* Mesh : WeaponMeshes)
    {
        if (!Mesh) continue;

        for (FName Tag : Mesh->ComponentTags)
        {
            FString TagString = Tag.ToString();
            FString SightType, IndexStr;

            if (TagString.Split(TEXT("_"), &SightType, &IndexStr, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
            {
                int32 Index = FCString::Atoi(*IndexStr);
                
                // TYPE A: Red Dot
                if (SightType.Equals(OpticPrefix, ESearchCase::IgnoreCase) && Mesh->DoesSocketExist(OpticSocket))
                {
                   FCachedSightData NewData;
                   NewData.bIsOptic = true;
                   NewData.SightIndex = Index;
                   NewData.Mesh = Mesh;
                   NewData.SocketNameA = OpticSocket;
                   CachedSights.Add(NewData);
                }
                // TYPE B: Iron Sights (Requires Rear Search)
                else if (SightType.Equals(FrontPrefix, ESearchCase::IgnoreCase) && Mesh->DoesSocketExist(FrontSocket))
                {
                    FCachedSightData NewData;
                    NewData.bIsOptic = false;
                    NewData.SightIndex = Index;
                    NewData.Mesh = Mesh;
                    NewData.SocketNameA = FrontSocket;

                    if (Mesh->DoesSocketExist(RearSocket))
                    {
                       NewData.SocketNameB = RearSocket; // Same mesh
                    }
                    else
                    {
                       // Hunt for separate rear sight mesh
                       FString TargetRearTag = RearPrefix + TEXT("_") + IndexStr;
                       for (UMeshComponent* PotentialRear : WeaponMeshes)
                       {
                          if (PotentialRear->ComponentHasTag(FName(*TargetRearTag)) && PotentialRear->DoesSocketExist(RearSocket))
                          {
                             NewData.SocketNameB = RearSocket;
                             NewData.RearMesh = PotentialRear;
                             break;
                          }
                       }
                    }
                    CachedSights.Add(NewData);
                }
            }
        }
    }

    CachedSights.Sort();
    
    // Resize Transforms array
    if (CachedSights.Num() > 0)
    {
        int32 MaxIndex = 0;
        for (const auto& Sight : CachedSights) MaxIndex = FMath::Max(MaxIndex, Sight.SightIndex);
        HandTransforms.SetNum(MaxIndex + 1);
    }
}

#pragma endregion Events & Start

float UFPSAnimInstance::GetPredictedStopDistance() const
{
	FVector PredictedVelocity = GetMovementComponent()->GetLastUpdateVelocity();
	bool bPredictedUseSeparateBrakingFriction = GetMovementComponent()->bUseSeparateBrakingFriction;
	float PredictedBrakingFriction = GetMovementComponent()->BrakingFriction;
	float PredictedGroundFriction = GetMovementComponent()->GroundFriction;
	float PredictedBrakingFrictionFactor = GetMovementComponent()->BrakingFrictionFactor;
	float PredictedBrakingDecelerationWalking = GetMovementComponent()->BrakingDecelerationWalking;
	
	return UAnimCharacterMovementLibrary::PredictGroundMovementStopLocation(
			PredictedVelocity,
			bPredictedUseSeparateBrakingFriction,
			PredictedBrakingFriction,
			PredictedGroundFriction,
			PredictedBrakingFrictionFactor,
			PredictedBrakingDecelerationWalking
		).Size();
}

bool UFPSAnimInstance::ShouldDistanceMatchStop() const
{
	return bHasVelocity && !bHasAcceleration;
}
