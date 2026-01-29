// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "FPS_Multiplayer/Public/Animation/FPSAnimInstance.h"
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
	// Concept: PlayRate = ActualSpeed / ReferenceAnimationSpeed
	
	if (GroundSpeed < 5.f)
	{
		PlayRate = 1.0f;
		return;
	}

	// 1. Determine what the "Reference Speed" should be based on current Gait.
	// This represents the speed the animation *wants* to move at.
	float ReferenceAnimSpeed = WalkSpeed;

	if (GaitValue >= RunGaitValue) 
	{
		// Between Run and Sprint
		ReferenceAnimSpeed = FMath::GetMappedRangeValueClamped(
			FVector2D(RunGaitValue, SprintGaitValue), 
			FVector2D(RunSpeed, SprintSpeed), 
			GaitValue
		);
	}
	else 
	{
		// Between Walk and Run
		ReferenceAnimSpeed = FMath::GetMappedRangeValueClamped(
			FVector2D(WalkGaitValue, RunGaitValue), 
			FVector2D(WalkSpeed, RunSpeed),
			GaitValue
		);
	}

	// 2. Calculate the ratio
	PlayRate = GroundSpeed / ReferenceAnimSpeed;

	// 3. Scale Adjustment (Pro Move)
	// If the character mesh is scaled (e.g. giant or dwarf), the stride length changes.
	if (FPSCharacter.IsValid())
	{
		const float ScaleZ = FPSCharacter->GetActorScale3D().Z; 
		if (!FMath::IsNearlyEqual(ScaleZ, 1.0f) && ScaleZ > 0.f)
		{
			PlayRate /= ScaleZ;
		}
	}

	// 4. Clamp to prevent visual artifacts (super fast legs or freeze-frame legs)
	PlayRate = FMath::Clamp(PlayRate, 0.5f, 2.0f);
}