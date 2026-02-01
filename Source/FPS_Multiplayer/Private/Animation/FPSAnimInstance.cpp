// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "FPS_Multiplayer/Public/Animation/FPSAnimInstance.h"

#include "Actor/Weapon/FPSWeapon.h"
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
		MovementDirectionMode = EMovementDirectionMode::Backward;
	}
	else if (bInNarrowRange)
	{
		// Definitely moving forwards (e.g. 0 deg)
		MovementDirectionMode = EMovementDirectionMode::Forward;
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

void UFPSAnimInstance::OnCharacterWeaponEquipped(AFPSWeapon* EquippedWeapon)
{
	if (!EquippedWeapon) return;

	// 1. Update the Enum
	EquippedWeaponState = EquippedWeapon->GetWeaponState();
	
	// 2. GET DATA: Pull the movement settings from the new weapon
	// (Assuming you added the struct and getter to FPSWeapon.h)
	const FWeaponMovementData& WeaponData = EquippedWeapon->GetMovementData();

	// 3. UPDATE ANIMATION REFS: 
	// Now CalculatePlayRate will use the correct math for THIS specific gun's animations.
	WalkSpeed   = WeaponData.AnimWalkRefSpeed;
	RunSpeed    = WeaponData.AnimRunRefSpeed;
	SprintSpeed = WeaponData.AnimSprintRefSpeed;
}
