// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "FPSAnimInstance.generated.h"

class UCharacterMovementComponent;
class AFPSPlayerCharacter;
/**
 * 
 */
UCLASS()
class FPS_MULTIPLAYER_API UFPSAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	
	// Like BeginPlay
	virtual void NativeInitializeAnimation() override;
	void CalculateGaitValue();

	// Like Tick (runs every frame)
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
protected:
	
	// --- REFERENCES (Memory Safety) ---
	// TWeakObjectPtr detects if the object died (GC'd). 
	// Prevents crashes when the character is destroyed but anim updates one last time.
	UPROPERTY(BlueprintReadOnly, Category = "References")
	TWeakObjectPtr<AFPSPlayerCharacter> FPSCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "References")
	TWeakObjectPtr<UCharacterMovementComponent> MovementComponent;
	
	// --- ANIMATION VARIABLES (Thread Safe) ---
	// The AnimGraph will read these directly without logic.
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float GroundSpeed;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsFalling;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsAccelerating; // Are pressing WASD?
	
	// Used for Strafing (Blend Space direction)
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Direction;
	
	// --- TRUE FPS VARIABLES ---
	
	// 0.0 = Idle, 1.0 = Walk, 2.0 = Run, 3.0 = Sprint
	UPROPERTY(BlueprintReadOnly, Category = "Movement | TrueFPS")
	float GaitValue;

	// Multiplier for animation speed (e.g. 0.8 to slow down, 1.2 to speed up)
	// Prevents "Ice Skating" foot slide.
	UPROPERTY(BlueprintReadOnly, Category = "Movement | TrueFPS")
	float PlayRate = 1.0f;

	// Used to start animations at the correct frame for instant response
	UPROPERTY(BlueprintReadOnly, Category = "Movement | TrueFPS")
	float StartPosition;
	
	// --- THRESHOLDS ---
	UPROPERTY(EditAnywhere, Category = "Movement")
	float WalkSpeed = 150.f; // Max Walking Speed
	
	UPROPERTY(EditAnywhere, Category = "Movement")
	float RunSpeed = 300.f;  // Max Running Speed
	
	UPROPERTY(EditAnywhere, Category = "Movement")
	float SprintSpeed = 600.f; // Max Sprinting Speed
};
