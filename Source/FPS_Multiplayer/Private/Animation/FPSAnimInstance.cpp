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

void UFPSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	// 1. Safety Check: Ensure character exists before trying to access data.
	if (!FPSCharacter.IsValid())
	{
		UpdateReferences();
		if (!FPSCharacter.IsValid()) return;
	}

	// 2. Execute Logic Chain
	CalculateEssentialData();
	CalculateLocomotionDirection();
	CalculateMovementDirectionMode();
	CalculateGaitValue();
	CalculatePlayRate();
	CalculatePitchValuePerBone();
	CalculateSightAlignment();
}

void UFPSAnimInstance::UpdateReferences()
{
	APawn* PawnOwner = TryGetPawnOwner();
	if (PawnOwner)
	{
		FPSCharacter = Cast<AFPSPlayerCharacter>(PawnOwner);
		if (FPSCharacter.IsValid())
		{
			FPSCharacter->GetCombatComponent()->OnWeaponEquippedDelegate.AddDynamic(this, &ThisClass::OnCharacterWeaponEquipped);
			MovementComponent = Cast<UCharacterMovementComponent>(FPSCharacter->GetMovementComponent());
		}
	}
}

// =========================================================================
//                           CALCULATIONS
// =========================================================================

void UFPSAnimInstance::CalculateEssentialData()
{
	// -- Velocity & Speed --
	Velocity = MovementComponent.Get()->Velocity;
	GroundSpeed = FVector(Velocity.X, Velocity.Y, 0.f).Size(); // Ignore Z for locomotion

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
}

void UFPSAnimInstance::CalculateLocomotionDirection()
{
	// Don't update direction if standing still to prevent the value from snapping to 0.
	if (GroundSpeed < 100.0f || !FPSCharacter.IsValid()) return;

	// Calculate direction of movement relative to the character's facing direction.
	// Result: -180 to 180 degrees.
	FRotator ActorRotation = FPSCharacter.Get()->GetActorRotation();
	FRotator VelocityRotation = UKismetMathLibrary::MakeRotFromX(Velocity);
    
	Direction = UKismetMathLibrary::NormalizedDeltaRotator(VelocityRotation, ActorRotation).Yaw;
	LocomotionDirection = Direction;
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
	// Map current Speed to a Gait Value (0=Idle, 1=Walk, 2=Run, 3=Sprint)
	
	if (GroundSpeed > RunSpeed)
	{
		// Blending Run to Sprint (2.0 -> 3.0)
		GaitValue = FMath::GetMappedRangeValueClamped(
			FVector2D(RunSpeed, SprintSpeed),
			FVector2D(RunGaitValue, SprintGaitValue), 
			GroundSpeed
		);
	}
	else if (GroundSpeed > WalkSpeed && GroundSpeed <= RunSpeed)
	{
		// Blending Walk to Run (1.0 -> 2.0)
		GaitValue = FMath::GetMappedRangeValueClamped(
			FVector2D(WalkSpeed, RunSpeed), 
			FVector2D(WalkGaitValue, RunGaitValue), 
			GroundSpeed
		);
	}
	else
	{
		// Blending Stop to Walk (0.0 -> 1.0)
		GaitValue = FMath::GetMappedRangeValueClamped(
			FVector2D(0.f, WalkSpeed), 
			FVector2D(0.0f, WalkGaitValue), 
			GroundSpeed
		);
	}
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
	// 1. SOURCE SELECTION (Network Logic)
	FRotator TargetRotation;
	
	if (FPSCharacter->IsLocallyControlled())
	{
		// The control pitch already comes clamped to avoid looking more than 90 deg up or 90 deg down 
		// Local Input (Instant, Smooth)
		TargetRotation = FPSCharacter->GetControlRotation();
	}
	else
	{
		// Replicated Data (Networked)
		// This reads the compressed 'RemoteViewPitch' that Unreal replicates automatically.
		// Replicated, compressed byte (might be jittery)
		TargetRotation = FPSCharacter->GetBaseAimRotation();
	}
	
	// 2. NORMALIZE
	// GetBaseAimRotation often returns 0-360, while ControlRotation is -90 to 90.
	// NormalizeAxis forces everything into -180 to 180 range to prevent flipping bugs.
	FRotator NormalizedTarget = TargetRotation;
	NormalizedTarget.Pitch = FRotator::NormalizeAxis(TargetRotation.Pitch);
	
	// 3. APPLY YOUR MATH (The 180 flip)
	// We compose the rotation to get the "Look Up/Down" angle relative to the spine's forward.
	// The control pitch goes from 0 to 90 deg when looking up, and from 360 to 270 when looking down... 
	// So we rotate it by 180 deg to make it go from 0 to -90 (up) and from 0 to 90 deg (down), ready to be applied to the bones...
	FRotator AimRotation = UKismetMathLibrary::ComposeRotators(NormalizedTarget, FRotator(180.0f, 0.f, 0.f));
	
	// 4. DEFINE THE GOAL
	// We want the final bone rotation to be 1/8th of the total look angle.
	// Divide by 8 bones (head, neck_01, neck_02, spine_05, spine_04, spine_03, spine_02 and spine_01)
	FRotator TargetPitchPerBone = FRotator(0.f, 0.f, AimRotation.Pitch / 8.0f);
	
	// 5. INTERPOLATE (Smoothing)
    // Smoothly transition from Current Value -> Target Value
    PitchValuePerBone = FMath::RInterpTo(
        PitchValuePerBone,      // Current
        TargetPitchPerBone,     // Target
        GetDeltaSeconds(),      // Time since last frame (UAnimInstance provides this)
        AimInterpSpeed          // Speed
    );
}

void UFPSAnimInstance::CalculateSightAlignment()
{
	// Add this check at the very top:
	if (LayerStates.AimState != EAimState::EAS_ADS)
	{
		// Reset transform smoothly back to zero if needed, or just return
		SightTransform = UKismetMathLibrary::TInterpTo(SightTransform, FTransform::Identity, GetDeltaSeconds(), 10.f);
		return;
	}
	
	if (FPSCharacter->IsLocallyControlled())
	{
		// Check if the character and its components are valid
		if (!FPSCharacter.IsValid() || !FPSCharacter->IsLocallyControlled()) return;

		// Get the Camera (Target)
		FTransform CameraTransform = FPSCharacter->GetCameraComponent()->GetComponentTransform();

		// Get the Gun Sight (Current Source)
		// Ensure you check pointers! GetEquippedWeapon() might be null.
		AFPSWeapon* Weapon = FPSCharacter->GetCombatComponent()->GetEquippedWeapon();
		if (!Weapon) return;

		FTransform SocketTransform = Weapon->GetWeaponMesh()->GetSocketTransform("GunSightSocket");

		// Calculate the Delta: "How do I get from the Socket TO the Camera?"
		FTransform RelativeTransform = UKismetMathLibrary::MakeRelativeTransform(CameraTransform, SocketTransform);

		// INTERPOLATION FIX:
		// 1. Use TInterpTo (Transform Interpolation)
		// 2. Pass 'SightTransform' (member var) as Current, NOT SocketTransform.
		SightTransform = UKismetMathLibrary::TInterpTo(
			SightTransform,     // Current: Where we were last frame
			CameraTransform,  // Target: Where we want to be
			GetDeltaSeconds(),  // Delta Time
			10.f                // Interp Speed
		);
	}
}

void UFPSAnimInstance::OnCharacterWeaponEquipped(AFPSWeapon* NewWeapon)
{
	// 1. Safety Check
	if (!IsValid(NewWeapon))
	{
		// Optional: Reset to defaults if weapon is null
		return;
	}

	// 2. GET DATA
	// We strictly use this event to update MATH CONSTANTS (Stride Warping).
	const FWeaponMovementData& WeaponData = NewWeapon->GetMovementData();

	// 3. UPDATE ANIMATION CONFIGURATION
	WalkSpeed   = WeaponData.AnimWalkRefSpeed;
	RunSpeed    = WeaponData.AnimRunRefSpeed;
	SprintSpeed = WeaponData.AnimSprintRefSpeed;

	// CRITICAL: DO NOT set LayerStates.OverlayState here!
	// Why? Because NativeUpdateAnimation() runs every frame and copies the 
	// authoritative state from the Character. If you set it here, it will 
	// just get overwritten 1 frame later.
}
