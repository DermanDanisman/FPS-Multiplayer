// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "FPS_Multiplayer/Public/Animation/FPSAnimInstance.h"

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

    // 1. Gather Data
    CalculateEssentialData();
    CalculateLocationData(DeltaSeconds);
    
    // 2. Calculate Math
    CalculatePitchValuePerBone();
    CalculateAimOffset();
    CalculateTurnInPlace();
    CalculateLocomotionState();
    
    // 3. Calculate IK / Transforms
    CalculateHandTransforms();     // Procedural ADS
    CalculateLeftHandTransform();  // Left Hand IK
	
	// --- THE INJECTION ---
	// This makes the AnimGraph "think" an animation is playing this curve.
	// Now "GetCurveValue(LocomotionState)" will return your value!
    
	// Note: We use 'AddCurveValue' because it's the public API. 
	// It works perfectly for this purpose.
	AddCurveValue(FName("LocomotionState"), LocomotionStateValue); 
}

#pragma endregion Lifecycle

#pragma region Data Gathering

// =========================================================================
//                           CALCULATIONS: DATA
// =========================================================================

void UFPSAnimInstance::CalculateEssentialData()
{
    // -- Velocity & Speed --
    // CRITICAL FOR REPLICATION:
    // Acceleration is replicated via the GetMovementComponent(). Input is not.
    Acceleration = GetMovementComponent()->GetCurrentAcceleration();
    Velocity = GetFPSCharacter()->GetVelocity();
    Velocity2D = FVector(Velocity.X, Velocity.Y, 0.f);
    
    Speed3D = Velocity.Size();
    GroundSpeed = Velocity2D.Size(); // Ignore Z for locomotion
    
    // NOTE: GetLastInputVector() is only valid Locally! Remote clients see (0,0,0).
    InputVector = GetMovementComponent()->GetLastInputVector(); 
    
    // -- State Booleans --
    bIsAccelerating = (Acceleration.SizeSquared2D() > 0.1f);
    bIsFalling = GetMovementComponent()->IsFalling();
    bIsCrouching = GetFPSCharacter()->IsCrouched();
    bCanJump = GetFPSCharacter()->CanJump();
    
    // "ShouldMove" gate: Only true if we have input AND reasonable speed.
    // Threshold (3.0f) prevents tiny micro-movements (analog drift) from triggering start animations.
    bShouldMove = bIsAccelerating && (GroundSpeed > 3.0f);
    
    // -- History Data (For future Stop/Pivot logic) --
    LastVelocityRotation = UKismetMathLibrary::MakeRotFromX(GetFPSCharacter()->GetVelocity());
    bHasMovementInput = InputVector.SizeSquared() > 0.f;
    LastMovementInputRotation = UKismetMathLibrary::MakeRotFromX(InputVector);
    MovementInputVelocityDifference = UKismetMathLibrary::NormalizedDeltaRotator(LastMovementInputRotation, LastVelocityRotation).Yaw;
    
    // --- SYNC LAYER STATES ---
    // The AnimInstance simply copies the Authoritative State from the Character.
    LayerStates = GetFPSCharacter()->GetLayerStates();
    bIsLocallyControlled = GetFPSCharacter()->IsLocallyControlled();
    
    // Cache Booleans for cleaner AnimGraph Transitions
    bIsSprinting = (LayerStates.Gait == EGait::EG_Sprinting);
    bIsAiming    = (LayerStates.AimState == EAimState::EAS_ADS || LayerStates.AimState == EAimState::EAS_Zoomed);
}

void UFPSAnimInstance::CalculateLocomotionStartDirection(const float StartAngle)
{
	if (UKismetMathLibrary::InRange_FloatFloat(StartAngle, -60.f, 60.f))
		LocomotionStartDirection = ELocomotionStartDirection::LSD_Forward;
	else if (UKismetMathLibrary::InRange_FloatFloat(StartAngle, 60.f, 120.f))
		LocomotionStartDirection = ELocomotionStartDirection::LSD_Right;
	else if (UKismetMathLibrary::InRange_FloatFloat(StartAngle, -120.f, -60.f))
		LocomotionStartDirection = ELocomotionStartDirection::LSD_Left;
	else if (UKismetMathLibrary::InRange_FloatFloat(StartAngle, 121.f, 180.f))
		LocomotionStartDirection = ELocomotionStartDirection::LSD_BackwardRight;
	else
		LocomotionStartDirection = ELocomotionStartDirection::LSD_BackwardLeft;
}

void UFPSAnimInstance::CalculateLocationData(float DeltaTime)
{
    DisplacementSinceLastUpdate = UKismetMathLibrary::VSizeXY(GetOwningActor()->GetActorLocation() - WorldLocation);
    WorldLocation = GetOwningActor()->GetActorLocation();
    DisplacementSpeed = UKismetMathLibrary::SafeDivide(DisplacementSinceLastUpdate, DeltaTime);
    
    if (bIsFirstUpdate)
    {
       DisplacementSinceLastUpdate = 0.f;
       DisplacementSpeed = 0.f;
       bIsFirstUpdate = false;
    }
}

void UFPSAnimInstance::CalculateLocomotionState()
{
	if (!GetFPSCharacter()) return;
	
	// Dot Product determines if we are reversing direction (Pivoting)
    FVector NormalizedVelocity = Velocity.GetSafeNormal();
    FVector NormalizedAcceleration = Acceleration.GetSafeNormal();
    float DotProduct = FVector::DotProduct(NormalizedVelocity, NormalizedAcceleration);
    
	float TargetStateValue = 0.0f;
	
    // State Machine Logic
	if (DotProduct < 0.0f)
	{
		LocomotionState = ELocomotionState::ELS_Idle; // ELocomotionState::ELS_Pivoting
	}
	else if (LayerStates.Gait == EGait::EG_Running 
		&& GroundSpeed > 1.0f && Acceleration.Size() > 400.0f 
		&& GetMovementComponent()->MaxWalkSpeed > GetMovementComponent()->GetWalkSpeed()) // This means we are in Running State
	{
		TargetStateValue = 2.0f;
		LocomotionState = ELocomotionState::ELS_Run;
	}
	else if (LayerStates.Gait == EGait::EG_Walking && GroundSpeed > 1.0f && Acceleration.Size() > 0.01f && GetMovementComponent()->MaxWalkSpeed > 1.0f)
	{
		TargetStateValue = 1.0f;
		LocomotionState = ELocomotionState::ELS_Walk;
	}
	else
	{
		TargetStateValue = 0.0f;
		LocomotionState = ELocomotionState::ELS_Idle;

	}
	
	// Interpolate simply to smooth out network jitter
	// If you want it instant (snappy), just set it. 
	// If you want it smooth like the video, Interp it.
	// 10.0f is a good speed: fast enough to feel responsive, smooth enough to blend.
	LocomotionStateValue = FMath::FInterpConstantTo(LocomotionStateValue, TargetStateValue, GetDeltaSeconds(), LocomotionCurveInterpSpeed);
} 

#pragma endregion Data Gathering

#pragma region Math & Physics

// =========================================================================
//                        CALCULATIONS: MATH & PHYSICS
// =========================================================================

void UFPSAnimInstance::CalculatePitchValuePerBone()
{
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

void UFPSAnimInstance::CalculateAimOffset()
{
	// 1. Get fundamental rotations
	FRotator ControlRotation = GetFPSCharacter()->GetReplicatedControlRotation();
	FRotator ActorRotation = GetFPSCharacter()->GetActorRotation();
    
	// 2. Calculate Pitch (Standard Look Up/Down)
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(ControlRotation, ActorRotation);
	PitchOffset = DeltaRot.Pitch;

	// 3. Handle Root Yaw (The Feet)
	// If we are moving, the feet must align with the body immediately.
	if (GroundSpeed > 5.0f || bIsFalling) 
	{
		RootYawOffset = 0.f;
		TurnInPlace = ETurnInPlace::ETIP_None;
        
		// Reset rotation history so we don't snap when we stop
		LastMovingRotation = ActorRotation; 
		MovingRotation = ActorRotation;
	}
	else // WE ARE STANDING STILL
	{
		LastMovingRotation = MovingRotation;
		MovingRotation = ActorRotation;

		// Calculate how much the Capsule (Actor) rotated this frame
		FRotator DeltaActorRot = UKismetMathLibrary::NormalizedDeltaRotator(MovingRotation, LastMovingRotation);
        
		// COUNTER-ROTATE: 
		// If the capsule turned 5 degrees Right, subtract 5 from RootYawOffset 
		// to keep the mesh looking like it stayed in the same world rotation.
		const float YawDelta = DeltaActorRot.Yaw;
        
		// We accumulate the offset. 
		// NOTE: We wrap this between -180 and 180 to prevent math errors on full spins.
		RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - YawDelta);

		// 4. Check if we need to turn
		CalculateTurnInPlace();
	}
    
	// 5. Set YawOffset for Blendspaces (optional, depending on your setup)
	YawOffset = RootYawOffset * -1.f;
}

void UFPSAnimInstance::CalculateTurnInPlace()
{
	// Threshold to trigger the turn animation (e.g., 90 degrees)
	// You can make this smaller (e.g., 45 or 60) for more responsive feet.
	const float TurnThreshold = TurnAngle; 

	// CASE 1: We are not currently turning, checking if we should start.
	if (TurnInPlace == ETurnInPlace::ETIP_None)
	{
		if (FMath::Abs(RootYawOffset) > TurnThreshold)
		{
			if (RootYawOffset > 0.f)
			{
				TurnInPlace = ETurnInPlace::ETIP_Right;
			}
			else
			{
				TurnInPlace = ETurnInPlace::ETIP_Left;
			}
		}
	}
    
	// CASE 2: We ARE turning.
	// Logic: We interpolate the Offset back to 0. 
	// As RootYawOffset approaches 0, the "Rotate Root Bone" node in AnimGraph 
	// applies less rotation, making the feet align with the capsule.
	if (TurnInPlace != ETurnInPlace::ETIP_None)
	{
		// Interp to 0
		RootYawOffset = FMath::FInterpTo(RootYawOffset, 0.f, GetDeltaSeconds(), TurnInterpSpeed);

		// If we are close enough to 0, we are done turning.
		if (FMath::Abs(RootYawOffset) < 10.f) // 10 degrees tolerance
		{
			TurnInPlace = ETurnInPlace::ETIP_None;
		}
	}
}

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

void UFPSAnimInstance::UpdateOnLocomotionEnter()
{
    if (!GetFPSCharacter()) return;

    StartRotation = GetFPSCharacter()->GetActorRotation();
    FVector DirectionVector;

    // A. Locally Controlled: Input is the "Intent" (Most responsive)
    if (GetFPSCharacter()->IsLocallyControlled())
    {
       DirectionVector = InputVector; 
       if (DirectionVector.IsNearlyZero()) DirectionVector = Velocity;
    }
    // B. Remote: Velocity is the "Reality" (Most accurate for network)
    else
    {
       DirectionVector = Acceleration;
       if (DirectionVector.IsNearlyZero()) DirectionVector = Velocity;
    }

    if (DirectionVector.IsNearlyZero()) return;

    PrimaryRotation = DirectionVector.ToOrientationRotator();
    SecondaryRotation = PrimaryRotation;
    
    FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(PrimaryRotation, StartRotation);
    LocomotionStartAngle = DeltaRotation.Yaw;

    CalculateLocomotionStartDirection(LocomotionStartAngle);
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