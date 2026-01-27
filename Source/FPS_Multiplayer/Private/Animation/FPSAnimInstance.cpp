// © 2025 Heathrow (Derman Can Danisman). All rights reserved.


#include "FPS_Multiplayer/Public/Animation/FPSAnimInstance.h"

#include "FPS_Multiplayer/Public/Character/FPSPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UFPSAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	// 1. Get the Owner
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

void UFPSAnimInstance::CalculateGaitValue()
{
	// --- CALCULATE GAIT VALUE ---
	// Logic: Map Speed to Gait (0=Idle, 1=Walk, 2=Run, 3=Sprint)

	if (GroundSpeed > RunSpeed)
	{
		// Map range from [Run -> Sprint] to [2.0 -> 3.0]
		GaitValue = FMath::GetMappedRangeValueClamped(
			FVector2D(RunSpeed, SprintSpeed), // Input Range
			FVector2D(2.0f, 3.0f),            // Output Range
			GroundSpeed                       // Value to Map
		);
	}
	else if (GroundSpeed > WalkSpeed)
	{
		// Map range from [Walk -> Run] to [1.0 -> 2.0]
		GaitValue = FMath::GetMappedRangeValueClamped(
			FVector2D(WalkSpeed, RunSpeed), 
			FVector2D(1.0f, 2.0f), 
			GroundSpeed
		);
	}
	else
	{
		// Map range from [0 -> Walk] to [0.0 -> 1.0]
		GaitValue = FMath::GetMappedRangeValueClamped(
			FVector2D(0.f, WalkSpeed), 
			FVector2D(0.0f, 1.0f), 
			GroundSpeed
		);
	}
}

void UFPSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	// 1. Safety Check (Smart Pointer)
	// If character is null or pending kill, stop.
	if (!FPSCharacter.IsValid() || !MovementComponent.IsValid())
	{
		// Try to init again (handle cases where Anim loads before Character)
		NativeInitializeAnimation();
		return;
	}
	
	// 2. GET SPEED
	FVector Velocity = MovementComponent->Velocity;
	Velocity.Z = 0.f; // Ignore vertical speed for walking anims
	GroundSpeed = Velocity.Size();

	// 3. GET FALLING STATE
	bIsFalling = MovementComponent->IsFalling();

	// 4. GET ACCELERATION (Are we pressing keys?)
	// Use CurrentAcceleration, not Velocity. 
	// (You can be moving but not accelerating if sliding on ice or stopping)
	bIsAccelerating = MovementComponent->GetCurrentAcceleration().Size() > 0.f;

	// 5. GET DIRECTION (For Strafing)
	// We need to know the angle between our Velocity and our Aim Rotation.
	FRotator AimRotation = FPSCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(Velocity);
    
	// Calculate Delta (Difference)
	// Normalized ensures result is between -180 and 180
	Direction = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
	
	// --- CALCULATE GAIT VALUE ---
	CalculateGaitValue();
}
