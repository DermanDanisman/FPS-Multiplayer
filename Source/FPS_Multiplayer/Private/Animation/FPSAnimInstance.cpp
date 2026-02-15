// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "FPS_Multiplayer/Public/Animation/FPSAnimInstance.h"

#include "Actor/Weapon/FPSWeapon.h"
#include "Camera/CameraComponent.h"
#include "Component/FPSCombatComponent.h"
#include "FPS_Multiplayer/Public/Character/FPSPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

// =========================================================================
//                           LIFECYCLE
// =========================================================================

void UFPSAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	UpdateReferences();
}

void UFPSAnimInstance::NativeUninitializeAnimation()
{
	if (FPSCharacter.IsValid())
	{
		if (UFPSCombatComponent* Combat = FPSCharacter->GetCombatComponent())
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
	
	// 1. Safety Check: Ensure character exists before trying to access data.
	if (!FPSCharacter.IsValid())
	{
		UpdateReferences();
		if (!FPSCharacter.IsValid()) return;
	}

	// 2. Data Gathering (Cheap variable copying)
	CalculateEssentialData();
	// CalculateLocomotionDirection();
	// CalculateCardinalDirection(); 
	// CalculateOrientationWarpingAngle();
	// CalculateMovementDirectionMode();
	// CalculateGaitValue();
	// CalculatePlayRate();
	CalculatePitchValuePerBone();
	CalculateAimOffset();
	CalculateTurnInPlace();
	CalculateHandTransforms();
	CalculateLeftHandTransform();
	
	CalculateLocomotionState();
}

void UFPSAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();
	
	// DISABLE THIS FOR FPS
	// This function forces the capsule to rotate towards the input key (WASD).
	// In FPS, we want the capsule locked to the Mouse Aim.
	// CalculateCharacterRotation();
}

void UFPSAnimInstance::UpdateReferences()
{
	APawn* PawnOwner = TryGetPawnOwner();
	if (PawnOwner)
	{
		FPSCharacter = Cast<AFPSPlayerCharacter>(PawnOwner);
		if (FPSCharacter.IsValid())
		{
			// 1. SAFETY CHECK: Ensure Combat Component exists
			UFPSCombatComponent* Combat = FPSCharacter->GetCombatComponent();
			if (Combat)
			{
				// 2. SAFETY UNBIND: Prevent duplicate bindings or stale references
				// If we are re-initializing (e.g. changed weapon), remove the old bind first.
				Combat->OnWeaponEquipped.Remove(OnWeaponEquippedDelegateHandle);
                
				// 3. BIND
				OnWeaponEquippedDelegateHandle = Combat->OnWeaponEquipped.AddUObject(this, &ThisClass::OnCharacterWeaponEquipped);
			}

			MovementComponent = Cast<UCharacterMovementComponent>(FPSCharacter->GetMovementComponent());
            
			// ... Initialize your other cached variables here if needed ...
			if (Combat && Combat->GetEquippedWeapon())
			{
				// Optional: If we spawned late and weapon is ALREADY there, manually trigger the update
				// OnCharacterWeaponEquipped(Combat->GetEquippedWeapon());
			}
		}
	}
}

// =========================================================================
//                           CALCULATIONS
// =========================================================================

void UFPSAnimInstance::CalculateEssentialData()
{
	// -- Velocity & Speed --
	// CRITICAL FOR REPLICATION:
	// Acceleration is replicated via the MovementComponent. Input is not.
	if (MovementComponent.IsValid())
	{
		Acceleration = MovementComponent->GetCurrentAcceleration();
		Velocity = MovementComponent->Velocity;
		Speed3D = Velocity.Size();
        
		// This is only valid Locally! Remote clients see (0,0,0)
		InputVector = MovementComponent->GetLastInputVector(); 
		
		Velocity2D = FVector(Velocity.X, Velocity.Y, 0.f);
		GroundSpeed = Velocity2D.Size(); // Ignore Z for locomotion
	}

	// -- State Booleans --
	bIsAccelerating = (MovementComponent.Get()->GetCurrentAcceleration().SizeSquared() > 0.f);
	bIsFalling = MovementComponent.Get()->IsFalling();
	bIsCrouching = FPSCharacter->IsCrouched();
	bCanJump = FPSCharacter->CanJump();
    
	// "ShouldMove" gate: Only true if we have input AND reasonable speed.
	// Threshold (100.0f) prevents tiny micro-movements from triggering start animations.
	bShouldMove = bIsAccelerating && (GroundSpeed > 100.0f);
	
	// -- History Data (For future Stop/Pivot logic) --
	LastVelocityRotation = UKismetMathLibrary::MakeRotFromX(FPSCharacter->GetVelocity());
	bHasMovementInput = MovementComponent->GetLastInputVector().SizeSquared() > 0.f;
	LastMovementInputRotation = UKismetMathLibrary::MakeRotFromX(MovementComponent->GetLastInputVector());
	MovementInputVelocityDifference = UKismetMathLibrary::NormalizedDeltaRotator(LastMovementInputRotation, LastVelocityRotation).Yaw;
	
	// --- SYNC LAYER STATES ---
	// The AnimInstance simply copies the Authoritative State from the Character.
	// This ensures that if the Server says "Crouch", the Animation does it.
	LayerStates = FPSCharacter->GetLayerStates();
	bIsLocallyControlled = FPSCharacter->IsLocallyControlled();
	
	// [NEW] Cache Booleans for cleaner AnimGraph Transitions
	// It is much cheaper to check a boolean in the Graph than to switch on an Enum every frame
	bIsSprinting = (LayerStates.Gait == EGait::EG_Sprinting);
	bIsAiming    = (LayerStates.AimState == EAimState::EAS_ADS);
}

void UFPSAnimInstance::CalculateLocomotionDirection()
{
	// 1. Safety Gate
	if (GroundSpeed < 10.0f || !FPSCharacter.IsValid()) return;

	// 2. Calculate the Raw Target (Where we WANT to go)
	FRotator VelocityRotation = UKismetMathLibrary::MakeRotFromX(Velocity);
	FRotator ActorRotation = FPSCharacter->GetActorRotation();
    
	// This is the raw angle (-180 to 180)
	float TargetDirection = UKismetMathLibrary::NormalizedDeltaRotator(VelocityRotation, ActorRotation).Yaw;
    
	// Update the raw 'Direction' variable for debug/other uses
	Direction = TargetDirection;

	// 3. THE FIX: Smart Interpolation
	// We treat the float as a Rotator so Unreal handles the -180/180 wrap automatically.
    
	FRotator CurrentRotation = FRotator(0.0f, LocomotionDirection, 0.0f);
	FRotator TargetRotation  = FRotator(0.0f, TargetDirection, 0.0f);

	// RInterpTo finds the shortest path (e.g. 179 -> -179 is just 2 degrees, not 358)
	FRotator NewRotation = FMath::RInterpTo(
		CurrentRotation, 
		TargetRotation, 
		GetDeltaSeconds(), 
		5.0f // Lower = Smoother, Higher = Snappier (Start with 6.0)
	);

	LocomotionDirection = NewRotation.Yaw;
}

void UFPSAnimInstance::CalculateCardinalDirection()
{
	// 1. Get Absolute Direction (0 to 180)
    float AbsDirection = FMath::Abs(LocomotionDirection);
    
    // 2. Get the Current State (Memory from last frame)
    ECardinalDirection Current = CardinalDirection; 

    // 3. HYSTERESIS LOGIC
    // Only switch states if we cross a specific "Buffer Line".
    // This prevents the "45-degree Jitter" by requiring the player to push
    // to 50 degrees to leave Forward, but drop to 40 degrees to return.
    
    switch (Current)
    {
    case ECardinalDirection::ECD_Forward:
       // We are moving Forward. To switch to Right/Left, we must push past 50 degrees.
       if (AbsDirection > 50.f) 
       {
          CardinalDirection = (LocomotionDirection > 0.f) ? ECardinalDirection::ECD_Right : ECardinalDirection::ECD_Left;
       }
       break;

    case ECardinalDirection::ECD_Right:
       // We are moving Right. To switch back to Forward, we must drop below 40 degrees.
       if (AbsDirection < 40.f) 
       {
           CardinalDirection = ECardinalDirection::ECD_Forward;
       }
       // To switch to Backward, we must push past 140 degrees.
       else if (AbsDirection > 140.f) 
       {
           CardinalDirection = ECardinalDirection::ECD_Backward;
       }
       break;

    case ECardinalDirection::ECD_Left:
       // Identical logic to Right (Abs handles the negative sign)
       if (AbsDirection < 40.f) 
       {
           CardinalDirection = ECardinalDirection::ECD_Forward;
       }
       else if (AbsDirection > 140.f) 
       {
           CardinalDirection = ECardinalDirection::ECD_Backward;
       }
       break;

    case ECardinalDirection::ECD_Backward:
       // We are moving Backward. To switch to Side, we must drop below 130 degrees.
       if (AbsDirection < 130.f) 
       {
          CardinalDirection = (LocomotionDirection > 0.f) ? ECardinalDirection::ECD_Right : ECardinalDirection::ECD_Left;
       }
       break;
    
    default:
        // Failsafe: If state is invalid (first frame), force a default based on raw math.
        if (AbsDirection <= 45.f) CardinalDirection = ECardinalDirection::ECD_Forward;
        else if (AbsDirection >= 135.f) CardinalDirection = ECardinalDirection::ECD_Backward;
        else CardinalDirection = (LocomotionDirection > 0.f) ? ECardinalDirection::ECD_Right : ECardinalDirection::ECD_Left;
        break;
    }
}

void UFPSAnimInstance::CalculateOrientationWarpingAngle()
{
	// 1. Define the "Offset" of the current animation
	float CardinalOffset = 0.0f;

	switch (CardinalDirection)
	{
	case ECardinalDirection::ECD_Forward:
		CardinalOffset = 0.0f;
		break;
	case ECardinalDirection::ECD_Right:
		CardinalOffset = 90.0f;
		break;
	case ECardinalDirection::ECD_Left:
		CardinalOffset = -90.0f;
		break;
	case ECardinalDirection::ECD_Backward:
		CardinalOffset = 180.0f; 
		// Note: For Backward, the logic can be tricky depending on if your anim is -180 or 180.
		// Usually, LocomotionDirection is +/- 180, so 180 works best.
		break;
	}

	// 2. Subtract the Offset from the Input Direction
	float RawWarpAngle = LocomotionDirection - CardinalOffset;

	// 3. Normalize the result to -180 to 180 (Crucial for the Backwards crossover)
	OrientationWarpingAngle = UKismetMathLibrary::NormalizeAxis(RawWarpAngle);
}

void UFPSAnimInstance::CalculateMovementDirectionMode()
{
	// Hysteresis Logic: Prevents "Flickering" between Forward and Backward animations
	// when strafing exactly sideways (90 degrees).
	
	// Calculate Buffered Thresholds
	float WideMin = DirectionThresholdMin - Buffer; // e.g. -95
	float WideMax = DirectionThresholdMax + Buffer; // e.g.  95
	float NarrowMin = DirectionThresholdMin + Buffer; // e.g. -85
	float NarrowMax = DirectionThresholdMax - Buffer; // e.g.  85
    
	bool bInWideRange = UKismetMathLibrary::InRange_FloatFloat(Direction, WideMin, WideMax, true, true);
	bool bInNarrowRange = UKismetMathLibrary::InRange_FloatFloat(Direction, NarrowMin, NarrowMax, true, true);
    
	// Switch Logic
	if (!bInWideRange)
	{
		// Definitely moving backwards (e.g. 180 deg)
		MovementDirectionMode = EMovementDirectionMode::EMDM_Backward;
	}
	else if (bInNarrowRange)
	{
		// Definitely moving forwards (e.g. 0 deg)
		MovementDirectionMode = EMovementDirectionMode::EMDM_Forward;
	}
	// If in between (Grey Zone), keep the previous state.
}

void UFPSAnimInstance::CalculateGaitValue()
{
	// 1. Calculate the Raw Gait Value based on Speed
	// (This allows smooth blending as the character accelerates)
	float RawGait = 0.f;

	if (GroundSpeed > RunSpeed)
	{
		// Blending Run to Sprint (2.0 -> 3.0)
		RawGait = FMath::GetMappedRangeValueClamped(
			FVector2D(RunSpeed, SprintSpeed),
			FVector2D(RunGaitValue, SprintGaitValue), 
			GroundSpeed
		);
	}
	else if (GroundSpeed > WalkSpeed && GroundSpeed <= RunSpeed)
	{
		// Blending Walk to Run (1.0 -> 2.0)
		RawGait = FMath::GetMappedRangeValueClamped(
			FVector2D(WalkSpeed, RunSpeed), 
			FVector2D(WalkGaitValue, RunGaitValue), 
			GroundSpeed
		);
	}
	else
	{
		// Blending Stop to Walk (0.0 -> 1.0)
		RawGait = FMath::GetMappedRangeValueClamped(
			FVector2D(0.f, WalkSpeed), 
			FVector2D(0.0f, WalkGaitValue), 
			GroundSpeed
		);
	}
	
	// 2. STATE CLAMPING (The Polished Fix)
	// We limit the Gait Value based on the INTENT (The Enum).
	// This prevents "Floating Sprinting" if the character is pushed fast but isn't inputting sprint.
    
	float MaxAllowedGait = 3.0f; // Default to Sprint
    
	switch (LayerStates.Gait)
	{
	case EGait::EG_Walking:
		MaxAllowedGait = WalkGaitValue; // Cap at 1.0
		break;
	case EGait::EG_Running:
		MaxAllowedGait = RunGaitValue;  // Cap at 2.0
		break;
	case EGait::EG_Sprinting:
		MaxAllowedGait = SprintGaitValue; // Cap at 3.0
		break;
	default:
		MaxAllowedGait = RunGaitValue;
		break;
	}

	// We allow a small tolerance (0.1f) so blending doesn't snap instantly
	GaitValue = FMath::Min(RawGait, MaxAllowedGait + 0.1f);
}

void UFPSAnimInstance::CalculatePlayRate()
{
	// Stride Warping: Adjusts animation playback speed to prevent foot sliding.
	
	// 1. Safety Check
	if (GroundSpeed < 5.f || !FPSCharacter.IsValid())
	{
		PlayRate = 1.0f;
		return;
	}

	// 3. Calculate Individual Rates (The "Map Range Unclamped" Nodes)
	// If I am moving 300 units/sec, and Walk Anim is 150:
	// WalkRate = 300 / 150 = 2.0 (Play double speed)
	const float WalkRate = GroundSpeed / WalkSpeed;
	const float RunRate  = GroundSpeed / RunSpeed;
	const float SprintRate = GroundSpeed / SprintSpeed;

	// 4. Blend Rates based on Gait (The "Map Range Clamped" Node)
	// This matches the logic: "If Gait is 1.5, give me 50% WalkRate and 50% RunRate"
	float BlendedPlayRate = 1.0f;

	if (GaitValue <= 1.0f) // Walking
	{
		BlendedPlayRate = WalkRate;
	}
	else if (GaitValue <= 2.0f) // Blending Walk -> Run
	{
		BlendedPlayRate = FMath::GetMappedRangeValueClamped(
			FVector2D(1.f, 2.f),               // Input Range (Gait)
			FVector2D(WalkRate, RunRate),      // Output Range (Rates)
			GaitValue
		);
	}
	else // Blending Run -> Sprint
	{
		BlendedPlayRate = FMath::GetMappedRangeValueClamped(
			FVector2D(2.f, 3.f),               // Input Range (Gait)
			FVector2D(RunRate, SprintRate),    // Output Range (Rates)
			GaitValue
		);
	}

	// 5. Scale Adjustment (The "Divide" Node)
	const float ScaleZ = FPSCharacter->GetActorScale3D().Z;
	if (ScaleZ > 0.01f) // Avoid divide by zero
	{
		BlendedPlayRate /= ScaleZ;
	}

	// 6. Set Final Result
	PlayRate = BlendedPlayRate;
}

// Calculate up/down rotation for spine, neck and head bones when looking up/down
void UFPSAnimInstance::CalculatePitchValuePerBone()
{
	// 1. Get the Rotations
	FRotator ControlRotation = FPSCharacter->GetReplicatedControlRotation();
	FRotator ActorRotation = FPSCharacter->GetActorRotation();
    
	// 2. Find the Local Aim Offset
	// This gives us a clean -90 to 90 Pitch value, completely ignoring world compass direction!
	FRotator AimOffset = UKismetMathLibrary::NormalizedDeltaRotator(ControlRotation, ActorRotation);
    
	// 3. Divide by 8 bones and invert. 
	// (Multiplying by -1.0f achieves the exact same thing as your 180-degree flip, but safely).
	float PitchPerBone = (AimOffset.Pitch / 8.0f) * -1.0f; 
    
	// 4. Apply to the correct axis (You had it mapped to Roll, which is standard for Epic's mannequins)
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
	// [REPLICATED] Orient in the direction of the camera’s rotation.
	
	FRotator ControlRotation = FPSCharacter->GetReplicatedControlRotation();
	FRotator ActorRotation = FPSCharacter->GetActorRotation();
	FRotator AimOffset = UKismetMathLibrary::NormalizedDeltaRotator(ControlRotation, ActorRotation);
	PitchOffset = AimOffset.Pitch;
}

void UFPSAnimInstance::CalculateTurnInPlace()
{
	if (bIsFalling || GroundSpeed > 0)
	{
		RootYawOffset = FMath::FInterpTo(
			RootYawOffset, 
			0.0f, 
			GetDeltaSeconds(), 
			5.f
		);
		
		YawOffset = RootYawOffset * -1.f;
		
		MovingRotation = FPSCharacter->GetActorRotation();
		LastMovingRotation = MovingRotation;
	}
	else
	{
		// Calculate Root Yaw Offset
		LastRootYawOffset = RootYawOffset;
		if (MovingRotation == FRotator::ZeroRotator)
		{
			LastMovingRotation = FPSCharacter->GetActorRotation();
		}
		else
		{
			LastMovingRotation = MovingRotation;
		}
		
		MovingRotation = FPSCharacter->GetActorRotation();
		
		FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovingRotation, LastMovingRotation);
		float DeltaRootYawOffset = RootYawOffset - DeltaRot.Yaw;
		
		if (GetCurveValue("Turning") > 0.0f)
		{
			RootYawOffset = FMath::FInterpTo(
				LastRootYawOffset, 
				DeltaRootYawOffset, 
				GetDeltaSeconds(), 
				5.f
			);
		}
		else
		{
			RootYawOffset = DeltaRootYawOffset;
		}
		
		YawOffset = RootYawOffset * -1.f;
		
		// Is Turning?
		if (GetCurveValue("Turning") > 0.0f)
		{
			LastDistanceCurve = DistanceCurve;
			
			bool bIsSmallerThanZero = (RootYawOffset < 0.0f);
			if (RootYawOffset > 0.0f && !bIsSmallerThanZero)
			{
				DistanceCurve = GetCurveValue("DistanceCurve");
			}
			else
			{
				DistanceCurve = GetCurveValue("DistanceCurve") * -1.f;
			}
		}
		
		DeltaDistanceCurve = DistanceCurve - LastDistanceCurve;
		if (RootYawOffset > 0.0f)
		{
			RootYawOffset = RootYawOffset + DeltaDistanceCurve;
		}
		else
		{
			RootYawOffset = RootYawOffset + DeltaDistanceCurve;
		}
		
		// Calculate Yaw Excess
		AbsoluteRootYawOffset = FMath::Abs(RootYawOffset);
		if (AbsoluteRootYawOffset > TurnAngle)
		{
			YawExcess = AbsoluteRootYawOffset - TurnAngle;
			
			if (RootYawOffset > 0.0f)
			{
				RootYawOffset = RootYawOffset - YawExcess;
			}
			else
			{
				RootYawOffset = RootYawOffset + YawExcess;
			}
		}
		
		// Calculate Speed Turn
		float PreviousYawRate = LookingRotation.Yaw;
		LookingRotation = FPSCharacter->GetReplicatedControlRotation();
		float DeltaYawRate = LookingRotation.Yaw - PreviousYawRate;
		YawRate = UKismetMathLibrary::MapRangeClamped(
			FMath::Abs(DeltaYawRate / GetDeltaSeconds()), 
			120.f, 
			600.f, 
			1.f, 
			3.f
		);
	}
}

void UFPSAnimInstance::CalculateLeftHandTransform()
{
	// 1. Safety Checks
	if (!FPSCharacter.IsValid() || !FPSCharacter->GetCombatComponent()) return;
    
	AFPSWeapon* EquippedWeapon = FPSCharacter->GetCombatComponent()->GetEquippedWeapon();
	if (!EquippedWeapon || !EquippedWeapon->GetWeaponMesh()) return;

	// 2. Get the Socket Transform in World Space
	FTransform SocketTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), RTS_World);
    
	// 3. Transform World Space -> Bone Space (Relative to RIGHT HAND)
	FVector OutPosition;
	FRotator OutRotation;
    
	FPSCharacter->GetMesh()->TransformToBoneSpace(
	   FName("hand_r"), 
	   SocketTransform.GetLocation(), 
	   SocketTransform.Rotator(), // <--- THE FIX: Use the socket's actual rotation!
	   OutPosition, 
	   OutRotation
	);
    
	// 4. Set the Variable
	LeftHandTargetTransform.SetLocation(OutPosition);
	LeftHandTargetTransform.SetRotation(FQuat(OutRotation));
}

// =========================================================================
//                        PROCEDURAL AIMING (The Math)
// =========================================================================

void UFPSAnimInstance::CalculateHandTransforms()
{
    // =========================================================
    // 1. SAFETY & NETWORK OPTIMIZATION
    // =========================================================
    
    // Safety: Ensure character and weapon exist
    if (!FPSCharacter.IsValid()) return;
    
    // [OPTIMIZATION] IsLocallyControlled Check
    // We strictly ONLY run this expensive math for the player controlling this character.
    // We do NOT want to calculate gun alignment for other players (Simulated Proxies).
    // They just play standard animations.
    if (!FPSCharacter->IsLocallyControlled()) 
    {
        HandLocation = FVector::ZeroVector;
        HandRotation = FRotator::ZeroRotator;
        return;
    }

    // =========================================================
    // 2. PREPARE WORLD DATA (Must run every frame)
    // =========================================================
    
    // We capture these transforms every frame because the player is constantly moving.
    // We use World Space as the "Common Language" to compare Bones vs Components.
    FTransform HandWorldTransform = FPSCharacter->GetMesh()->GetSocketTransform(HandBoneName, RTS_World);
    //FTransform CameraWorldTransform  = FPSCharacter->GetCameraComponent()->GetComponentTransform();
    FTransform HeadWorldTransform = FPSCharacter->GetMesh()->GetSocketTransform(HeadBoneName, RTS_World);

    // Note: We use the CACHED float 'CurrentDistanceFromCamera' here instead of calling a function.
	
	// =========================================================
	// THE PROXY CAMERA FIX
	// =========================================================
	FTransform CameraWorldTransform;
    
	if (FPSCharacter->IsLocallyControlled())
	{
		// Local Player: Use the actual Camera Component
		CameraWorldTransform = FPSCharacter->GetCameraComponent()->GetComponentTransform();
	}
	else
	{
		// --- THE FIX ---
		// Proxy Player: Create a fake camera that points straight out of their face.
        
		// 1. Get the Network Pitch (Up/Down)
		float NetworkPitch = FPSCharacter->GetReplicatedControlRotation().Pitch;
        
		// 2. Get the Visual Yaw of the Head (Where the body is actually facing right now)
		float VisualYaw = HeadWorldTransform.Rotator().Yaw;
        
		// 3. Create the Fake Camera
		FRotator FakeCameraRot = FRotator(NetworkPitch, VisualYaw, 0.f);
		FVector HeadLoc = HeadWorldTransform.GetLocation();
        
		CameraWorldTransform = FTransform(FakeCameraRot, HeadLoc, FVector::OneVector);
	}

    // =========================================================
    // 3. ITERATE CACHED SIGHTS (The Optimization)
    // =========================================================
    
    // [OPTIMIZATION] Instead of "GetComponents" and "Split String" loops,
    // we iterate this clean array we built in OnCharacterWeaponEquipped.
    // This removes all memory allocation from the Tick loop.
    
    for (const FCachedSightData& Sight : CachedSights)
    {
        // Skip if the mesh was destroyed (weapon dropped/destroyed)
        if (!Sight.Mesh.IsValid()) continue;

        FTransform FinalTransform = FTransform::Identity;

        // --- LOGIC A: OPTIC SIGHTS (Red Dots) ---
        if (Sight.bIsOptic)
        {
            // 1. Get current World Location of the Red Dot socket
            FTransform SightTransform = Sight.Mesh->GetSocketTransform(Sight.SocketNameA, RTS_World);
            
            // 2. Calculate the Delta: Hand -> Sight
            FTransform HandToSight = UKismetMathLibrary::MakeRelativeTransform(HandWorldTransform, SightTransform);

            // 3. Apply Camera Distance (Cached)
            FVector AdjLoc = HandToSight.GetLocation();
            AdjLoc.X += CurrentDistanceFromCamera; 
            
            // 4. Calculate where the Hand needs to be in World Space
            FTransform AdjTrans(HandToSight.GetRotation(), AdjLoc, FVector::OneVector);
            FTransform NewHandWorldTransform = AdjTrans * CameraWorldTransform;
            
            // 5. Convert World Space -> Head Space (For AnimGraph)
            FinalTransform = UKismetMathLibrary::MakeRelativeTransform(NewHandWorldTransform, HeadWorldTransform);
        }
        // --- LOGIC B: IRON SIGHTS (Vector Math) ---
        else
        {
            // Note: For Iron Sights, we generally expect the Rear Sight to be static relative to the Front.
            // If you need the complex "Rear Component" search logic here, you should cache the 
            // RearComponent pointer in the FCachedSightData struct too.
            
            // This is the simplified version assuming Front/Rear are retrievable via the cached mesh/sockets.
            FVector FrontLocation = Sight.Mesh->GetSocketLocation(Sight.SocketNameA); // Front
        	
        	// Get Rear Transform
        	// CHECK: Is the rear sight on a separate mesh, or the main mesh?
        	UMeshComponent* RearMeshToUse = Sight.RearMesh.IsValid() ? Sight.RearMesh.Get() : Sight.Mesh.Get();
            
        	if (!RearMeshToUse) continue; // Safety
        	
            FTransform RearTransform = Sight.Mesh->GetSocketTransform(Sight.SocketNameB, RTS_World); // Rear
            
            FVector RearLocation = RearTransform.GetLocation();
            FRotator RearRotation = RearTransform.Rotator();

            // Vector math to align Front post perfectly with Rear notch
            FVector DirectionToTarget = (FrontLocation - RearLocation).GetSafeNormal();
            FVector UpVector = UKismetMathLibrary::GetUpVector(RearRotation);
            FVector RightVector = UKismetMathLibrary::GetRightVector(RearRotation);

            float UpDot = FVector::DotProduct(DirectionToTarget, UpVector);
            float RightDot = FVector::DotProduct(DirectionToTarget, RightVector);
            
            float AnglePitch = UKismetMathLibrary::DegAsin(UpDot);
            float AngleYaw = UKismetMathLibrary::DegAsin(RightDot);

            FRotator PitchRot = UKismetMathLibrary::RotatorFromAxisAndAngle(RightVector, -AnglePitch); 
            FRotator YawRot = UKismetMathLibrary::RotatorFromAxisAndAngle(UpVector, AngleYaw);

            FRotator CombinedCorrection = UKismetMathLibrary::ComposeRotators(PitchRot, YawRot);
            FRotator FinalSightRotation = UKismetMathLibrary::ComposeRotators(CombinedCorrection, RearRotation);

            FTransform TrueSightTransform(FinalSightRotation, RearLocation, FVector::OneVector);
            FTransform HandToSight = UKismetMathLibrary::MakeRelativeTransform(HandWorldTransform, TrueSightTransform);

            FVector AdjLoc = HandToSight.GetLocation();
            AdjLoc.X = CurrentDistanceFromCamera; // Cached
            
            FTransform AdjTrans(HandToSight.GetRotation(), AdjLoc, FVector::OneVector);
            FTransform NewHandWorld = AdjTrans * CameraWorldTransform;
            FinalTransform = UKismetMathLibrary::MakeRelativeTransform(NewHandWorld, HeadWorldTransform);
        }

        // Store the result directly into the array at the correct index
        if (HandTransforms.IsValidIndex(Sight.SightIndex))
        {
            HandTransforms[Sight.SightIndex] = FinalTransform;
        }
    }

    // =========================================================
    // 4. DETERMINE THE TARGET (The Decision)
    // =========================================================
    
    FTransform DesiredTarget = FTransform::Identity;

    // A. ARE WE AIMING? (ADS)
    if (LayerStates.AimState == EAimState::EAS_ADS || LayerStates.AimState == EAimState::EAS_Zoomed)
    {
        if (HandTransforms.IsValidIndex(CurrentSightIndex))
        {
            // Set target to the calculated ADS position
            DesiredTarget = HandTransforms[CurrentSightIndex];
        }
    }
    // B. ARE WE HIP FIRING? (Resting)
    else
    {
        // [OPTIMIZATION] Use the CACHED HipFireOffset.
        // We do not call Weapon->GetHipFireOffset() here anymore.
        
        // 1. Calculate World Position relative to Camera
        FTransform HipTargetWorldTransform = CurrentHipFireOffset * CameraWorldTransform;

        // 2. Convert to Head Space
        DesiredTarget = UKismetMathLibrary::MakeRelativeTransform(HipTargetWorldTransform, HeadWorldTransform);
    }
	
	// =========================================================
	// THE UNIFIED SMOOTHING (The Magic)
	// =========================================================
	// Whether we are aiming OR hip firing, we smooth towards the target.
	// This handles the transition (blending) automatically!
    
	CurrentHandTransform = UKismetMathLibrary::TInterpTo(
		CurrentHandTransform, 
		DesiredTarget, 
		GetDeltaSeconds(), 
		AimInterpSpeed
	);

	// =========================================================
	// OUTPUT TO ANIM GRAPH (Unified)
	// =========================================================
	// We use the SAME variables for both states.
    
	HandLocation = CurrentHandTransform.GetLocation();
	HandRotation = CurrentHandTransform.Rotator();
}

void UFPSAnimInstance::CalculateLocomotionState()
{
	FVector NormalizedVelocity = Velocity.GetSafeNormal();
	FVector NormalizedAcceleration = Acceleration.GetSafeNormal();
	
	/*// [CHANGE] Simplified Logic for robustness
	// If we are moving fast enough, we are running/walking
	if (GroundSpeed > RunSpeed - 10.f) 
	{
		LocomotionState = ELocomotionState::ELS_Run;
	}
	else if (GroundSpeed > WalkSpeed + 10.f)
	{
		LocomotionState = ELocomotionState::ELS_Run; // Or Walk, depending on your setup
	}
	else if (GroundSpeed > 10.f)
	{
		LocomotionState = ELocomotionState::ELS_Walk;
	}
	else
	{
		LocomotionState = ELocomotionState::ELS_Idle;
	}
    
	// Only apply the "Pivot" check if we are locally controlled or have high confidence
	// because Acceleration is noisy on clients.
	if (FPSCharacter->IsLocallyControlled())
	{
		float DotProduct = FVector::DotProduct(NormalizedVelocity, NormalizedAcceleration);
		if (GroundSpeed > 200.f && DotProduct < -0.5f)
		{
			LocomotionState = ELocomotionState::ELS_Pivoting;
		}
	}*/
	
	float DotProduct = FVector::DotProduct(NormalizedVelocity, NormalizedAcceleration);
	if (DotProduct < 0.0f)
	{
		LocomotionState = ELocomotionState::ELS_Idle; // ELocomotionState::ELS_Pivoting
	}
	else if (GroundSpeed > 1.0f && Acceleration.Size() > 400.0f && MovementComponent.Get()->MaxWalkSpeed > WalkSpeed) // This means we are in Running State
	{
		LocomotionState = ELocomotionState::ELS_Run;
	}
	else if (GroundSpeed > 1.0f && Acceleration.Size() > 0.01f && MovementComponent.Get()->MaxWalkSpeed > 1.0f)
	{
		LocomotionState = ELocomotionState::ELS_Walk;
	}
	else
	{
		LocomotionState = ELocomotionState::ELS_Idle;
	}
	
	/*// 1. Determine the New State based on data
    ELocomotionState NewState = ELocomotionState::ELS_Idle;

    if (GroundSpeed > RunSpeed - 10.f) // Buffer to prevent flickering near max speed
    {
        NewState = ELocomotionState::ELS_Sprint;
    }
    else if (GroundSpeed > WalkSpeed + 10.f)
    {
        NewState = ELocomotionState::ELS_Run;
    }
    else if (GroundSpeed > 10.f) // Small threshold to ignore micro-movement
    {
        NewState = ELocomotionState::ELS_Walk;
    }
    else
    {
        NewState = ELocomotionState::ELS_Idle;
    }

    // 2. Pivot Check (Optional logic)
    FVector NormalizedVelocity = Velocity.GetSafeNormal();
    FVector NormalizedAcceleration = Acceleration.GetSafeNormal();
    float DotProduct = FVector::DotProduct(NormalizedVelocity, NormalizedAcceleration);
    
    // If moving fast and acceleration opposes velocity -> Pivot
    if (GroundSpeed > 200.f && DotProduct < -0.5f) 
    {
       NewState = ELocomotionState::ELS_Pivoting;
    }

    // 3. Update the Variable
    LocomotionState = NewState;*/
}

void UFPSAnimInstance::CalculateCharacterRotation()
{
	if (LocomotionState == ELocomotionState::ELS_Idle)
	{
		
	}
	else
	{
		CalculateRotationWhileMoving();
	}
}

void UFPSAnimInstance::CalculateRotationWhileMoving()
{
	PrimaryRotation = UKismetMathLibrary::RInterpTo_Constant(
		PrimaryRotation, 
		UKismetMathLibrary::MakeRotFromX(InputVector), 
		GetDeltaSeconds(), 
		1000.f
	);
	
	SecondaryRotation = UKismetMathLibrary::RInterpTo(SecondaryRotation, PrimaryRotation, GetDeltaSeconds(), 8.f);
	
	float RotationCurve = GetCurveValue(FName("RotationCurve"));
	float RangeClampedRotationCurve = UKismetMathLibrary::MapRangeClamped(RotationCurve, 0.f, 1.f, 1.f, 0.f);
	float FinalLocomotionStartAngle = ((RangeClampedRotationCurve * -1.f) * LocomotionStartAngle) + SecondaryRotation.Yaw;
	
	FPSCharacter->SetActorRotation(UKismetMathLibrary::MakeRotator(0.f, 0.f, FinalLocomotionStartAngle));
}

void UFPSAnimInstance::UpdateOnRunEnter()
{
	if (!FPSCharacter.IsValid()) return;

	StartRotation = FPSCharacter->GetActorRotation();
    
	FVector DirectionVector = FVector::ZeroVector;

	// --- LOGIC SPLIT ---
    
	// A. Locally Controlled (Input is the most responsive "Intent")
	if (FPSCharacter->IsLocallyControlled())
	{
		DirectionVector = InputVector; 
		if (DirectionVector.IsNearlyZero()) DirectionVector = Velocity;
	}
	// B. Remote / Simulated Proxy (Velocity is the most accurate "Reality")
	else
	{
		// [CHANGE] Use Velocity first for Remote players. 
		// It aligns perfectly with the movement the client sees.
		DirectionVector = Velocity;

		// Fallback to Acceleration if Velocity is tiny (e.g. start of movement)
		if (DirectionVector.IsNearlyZero())
		{
			DirectionVector = Acceleration;
		}
	}

	// Safety
	if (DirectionVector.IsNearlyZero()) return;

	// --- CALCULATION ---
	PrimaryRotation = DirectionVector.ToOrientationRotator();
	SecondaryRotation = PrimaryRotation;
    
	FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(PrimaryRotation, StartRotation);
	LocomotionStartAngle = DeltaRotation.Yaw;
    
	// Debug to see what the Client calculates for the Server
	/*
	FString Role = FPSCharacter->HasAuthority() ? TEXT("Server") : TEXT("Client");
	FString Control = FPSCharacter->IsLocallyControlled() ? TEXT("Local") : TEXT("Remote");
	UE_LOG(LogTemp, Warning, TEXT("[%s][%s] Start Angle: %f | Vec: %s"), 
	   *Role, *Control, LocomotionStartAngle, *DirectionVector.ToString());
	*/

	CalculateLocomotionStartDirection(LocomotionStartAngle);
}

void UFPSAnimInstance::CalculateLocomotionStartDirection(const float StartAngle)
{
	if (UKismetMathLibrary::InRange_FloatFloat(StartAngle, -60.f, 60.f))
	{
		LocomotionStartDirection = ELocomotionStartDirection::LSD_Forward;
	}
	else if (UKismetMathLibrary::InRange_FloatFloat(StartAngle, 60.f, 120.f))
	{
		LocomotionStartDirection = ELocomotionStartDirection::LSD_Right;
	}
	else if (UKismetMathLibrary::InRange_FloatFloat(StartAngle, -120.f, -60.f))
	{
		LocomotionStartDirection = ELocomotionStartDirection::LSD_Left;
	}
	else if (UKismetMathLibrary::InRange_FloatFloat(StartAngle, 121.f, 180.f))
	{
		LocomotionStartDirection = ELocomotionStartDirection::LSD_BackwardRight;
	}
	else
	{
		LocomotionStartDirection = ELocomotionStartDirection::LSD_BackwardLeft;
	}
}

void UFPSAnimInstance::UpdateLocomotionPlayRate()
{
	// 1. PREFER ANIMATION CURVE
	// We try to pull the speed data directly from the animation asset.
	float ReferenceAnimSpeed = GetCurveValue(FName("MoveSpeed"));

	// 2. FALLBACK LOGIC (The "Anti-Crash" System)
	// If the curve is 0 (Idle, Transition, Missing Curve, or Curve Evaluation Disabled),
	// we MUST provide a valid number to divide by.
	if (ReferenceAnimSpeed <= 10.0f)
	{
		// Option A: Use the Character's actual movement limit (Dynamic)
		if (MovementComponent.IsValid())
		{
			ReferenceAnimSpeed = MovementComponent->GetMaxSpeed();
		}

		// Option B: Final Safety Net. 
		// If MovementComponent is null or MaxSpeed is 0 (Rooted/Stunned), force a generic value.
		// This guarantees "ReferenceAnimSpeed" is NEVER 0.
		if (ReferenceAnimSpeed <= 10.0f)
		{
			ReferenceAnimSpeed = 350.0f; // Standard Jog Speed
		}
	}

	// 3. CALCULATION
	// SafeDivide returns 0 if denominator is 0, but our logic above ensures that won't happen.
	// Formula: Actual Speed / Animation Speed = Play Rate multiplier.
	float CalculatedRate = UKismetMathLibrary::SafeDivide(GroundSpeed, ReferenceAnimSpeed);

	// 4. CLAMP (CRITICAL)
	// We limit the rate. 
	// - 0.5f prevents freezing (0.0).
	// - 2.0f prevents exploding (Infinity).
	LocomotionPlayRate = UKismetMathLibrary::Clamp(CalculatedRate, 0.75f, 1.75f);

	// 5. STOPPED RESET
	// If the character is effectively stopped, force the rate to 1.0.
	// This ensures that when you START moving again, the "Start" animation plays at normal speed.
	if (GroundSpeed < 10.0f)
	{
		LocomotionPlayRate = 1.0f;
	}
}

void UFPSAnimInstance::OnCharacterWeaponEquipped(AFPSWeapon* NewWeapon)
{
	if (!NewWeapon) 
	{
		// Reset to defaults or safe values
		CurrentHipFireOffset = FTransform::Identity;
		HandTransforms.Empty();
		CachedSights.Empty();
		return; 
	}
	
	bIsArmed = IsValid(NewWeapon);
	
	// 1. Cache Simple Data (The stuff you noticed!)
    const FWeaponMovementData& WeaponData = NewWeapon->GetMovementData();
    WalkSpeed   = WeaponData.AnimWalkRefSpeed;
    RunSpeed    = WeaponData.AnimRunRefSpeed;
    SprintSpeed = WeaponData.AnimSprintRefSpeed;

    CurrentHipFireOffset = NewWeapon->GetHipFireOffset();
    CurrentDistanceFromCamera = NewWeapon->GetDistanceFromCamera();
	CurrentRightHandEffectorLocation = NewWeapon->GetRightHandEffectorLocation();
	CurrentRightHandJointTargetLocation = NewWeapon->GetRightHandJointTargetLocation();
    TimeToAim = NewWeapon->GetTimeToAim();
    TimeFromAim = NewWeapon->GetTimeFromAim();

    // 2. BUILD THE SIGHT CACHE (The String Parsing Optimization)
    CachedSights.Empty();
    
    TArray<UMeshComponent*> WeaponMeshes;
    NewWeapon->GetComponents<UMeshComponent>(WeaponMeshes);

	const FName OpticSocket = NewWeapon->GetOpticSocketName();
	const FName FrontSocket = NewWeapon->GetFrontSightSocketName();
	const FName RearSocket  = NewWeapon->GetRearSightSocketName(); // "RearAimpoint"
    
	const FString OpticPrefix = NewWeapon->GetOpticTagPrefix();
	const FString FrontPrefix = NewWeapon->GetFrontSightTagPrefix();
	const FString RearPrefix  = NewWeapon->GetRearSightTagPrefix(); // "RearSight"

    // Heavy Loop: Runs ONCE per weapon equip.
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

            	// --- TYPE A: RED DOT / SCOPE ---
            	if (SightType.Equals(OpticPrefix, ESearchCase::IgnoreCase) && Mesh->DoesSocketExist(OpticSocket))
            	{
            		FCachedSightData NewData;
            		NewData.bIsOptic = true;
            		NewData.SightIndex = Index;
            		NewData.Mesh = Mesh;
            		NewData.SocketNameA = OpticSocket;
            		CachedSights.Add(NewData);
            	}
            	// --- TYPE B: IRON SIGHTS ---
            	// Entry Point: We MUST find the Front Sight tag first.
                else if (SightType.Equals(FrontPrefix, ESearchCase::IgnoreCase) && Mesh->DoesSocketExist(FrontSocket))
                {
                    // For Iron Sights, we need to find the rear sight component too. 
                    // (You can perform the Rear Sight Search logic here and store it in NewData.SocketNameB/Mesh)
                    // For brevity, I'm assuming you implement the Rear Search here similarly to your old code.
                    
                	FCachedSightData NewData;
                	NewData.bIsOptic = false;
                	NewData.SightIndex = Index;
                	NewData.Mesh = Mesh; // Front Sight is here
                	NewData.SocketNameA = FrontSocket;
                	// SMART CHECK: Is the Rear Sight on THIS SAME MESH?
                	if (Mesh->DoesSocketExist(RearSocket))
                	{
                		// YES! Standard Rifle. Both sockets on one mesh.
                		NewData.SocketNameB = RearSocket;
                		NewData.RearMesh = nullptr; // nullptr means "Use Main Mesh"
                	}
                	else
                	{
                		// NO! The rear sight must be a separate attachment.
                		// Let's hunt for it.
                		FString TargetRearTag = RearPrefix + TEXT("_") + IndexStr; // e.g., "RearSight_0"
                        
                		for (UMeshComponent* PotentialRear : WeaponMeshes)
                		{
                			if (PotentialRear->ComponentHasTag(FName(*TargetRearTag)) && PotentialRear->DoesSocketExist(RearSocket))
                			{
                				NewData.SocketNameB = RearSocket;
                				NewData.RearMesh = PotentialRear; // Found it!
                				break;
                			}
                		}
                	}
                    CachedSights.Add(NewData);
                }
            }
        }
    }

    // Sort sights so Index 0 is always first
    CachedSights.Sort();
    
    // Initialize HandTransforms array size
    if (CachedSights.Num() > 0)
    {
        // Find max index to size array correctly
        int32 MaxIndex = 0;
        for (const auto& Sight : CachedSights) MaxIndex = FMath::Max(MaxIndex, Sight.SightIndex);
        HandTransforms.SetNum(MaxIndex + 1);
    }
}