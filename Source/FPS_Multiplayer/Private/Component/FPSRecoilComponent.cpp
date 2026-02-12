// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "Component/FPSRecoilComponent.h"


UFPSRecoilComponent::UFPSRecoilComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UFPSRecoilComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UFPSRecoilComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// 1. Validate Weapon
	AActor* WeaponOwner = GetOwner();
	if (!WeaponOwner) return;
	
	// 2. Validate Character (The Weapon's Owner)
	// If weapon is dropped on the floor, it has no owner, so we stop logic.
	APawn* OwnerPawn = Cast<APawn>(WeaponOwner->GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled()) return;

	APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC) return;

	// --- 1. RECOVERY LOGIC ---
	if (bIsRecovering)
	{
		// Slowly move the Target back to Zero
		TargetRecoilRotation = FMath::RInterpTo(TargetRecoilRotation, FRotator::ZeroRotator, DeltaTime, CurrentRecoverySpeed);
	}

	// --- 2. INTERPOLATION LOGIC (The Smoother) ---
	// Move Current towards Target
	FRotator InterpRotation = FMath::RInterpTo(CurrentRecoilRotation, TargetRecoilRotation, DeltaTime, CurrentInterpSpeed);
    
	// Calculate the tiny step to move THIS frame
	FRotator DeltaRotation = InterpRotation - CurrentRecoilRotation;

	// Apply to View
	PC->AddPitchInput(DeltaRotation.Pitch);
	PC->AddYawInput(DeltaRotation.Yaw);

	// Update State
	CurrentRecoilRotation = InterpRotation;
}

void UFPSRecoilComponent::ApplyRecoil(const FVector& RecoilForce, float InterpSpeed, float RecoverySpeed)
{
	// 1. Update Config
	CurrentInterpSpeed = InterpSpeed;
	CurrentRecoverySpeed = RecoverySpeed;
	bIsRecovering = false; // We are shooting, so stop recovering!
	
	// 2. Add to Target (Accumulate the kick)
	TargetRecoilRotation.Pitch += RecoilForce.Y; 
	TargetRecoilRotation.Yaw += RecoilForce.X;
}

void UFPSRecoilComponent::StartRecovery()
{
	bIsRecovering = true;
}

