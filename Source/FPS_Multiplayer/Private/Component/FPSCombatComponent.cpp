// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "Component/FPSCombatComponent.h"

#include "Actor/Weapon/FPSWeapon.h"
#include "Character/FPSPlayerCharacter.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

static FString CombatStateToString(ECombatState State)
{
	if (const UEnum* Enum = StaticEnum<ECombatState>())
	{
		return Enum->GetDisplayNameTextByValue((int64)State).ToString();
	}
	return TEXT("Invalid");
}

// Sets default values for this component's properties
UFPSCombatComponent::UFPSCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// Correct Initialization for Components.
	// "SetReplicates(true)" causes crashes in Constructors. This is the safe alternative.
	SetIsReplicatedByDefault(true);
}

void UFPSCombatComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// 1. Gameplay Logic: Tell EVERYONE (so they see the gun).
	DOREPLIFETIME_CONDITION(ThisClass, EquippedWeapon, COND_None);
	DOREPLIFETIME_CONDITION(ThisClass, CombatState, COND_None);
	DOREPLIFETIME(ThisClass, CarriedAmmo);
}

void UFPSCombatComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UFPSCombatComponent::EquipWeapon(AFPSWeapon* WeaponToEquip)
{
	if (!IsValid(WeaponToEquip)) return;
	
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	
	// --- STEP 1: AUTOMATIC NETWORK ROUTING ---
	// If we are a Client, we cannot set variables. 
	// So, we automatically call the Server RPC and return.
	// The Blueprint user doesn't need to know this happened!
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_EquipWeapon(WeaponToEquip);
		return; // Our job on the Client is done. The Server takes over.
	}
	
	// --- SERVER LOGIC START ---
	if (OwnerCharacter)
	{
		// 1. Update State (Server Side)
		EquippedWeapon = WeaponToEquip;
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		
		// 2. Attach Visuals (Server Side)
		// The Server needs to see the attachment too!
		EquippedWeapon->AttachToComponent(
			OwnerCharacter->GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale, 
			EquippedWeapon->GetWeaponHandSocketName()
		);
		
		// Sets the initial clip ammo count to full when a weapon picked up 
		// (This will need proper adjustments because we cannot full the clip everytime we drop and pick up (new or already equipped) weapon)
		EquippedWeapon->SetInitialClipAmmo();
		
		// 3. Set Owner
		// Crucial for "IsLocallyControlled" checks and lag compensation later.
		EquippedWeapon->SetOwner(OwnerCharacter);
		
		// 5. Stat Updates
		if (WeaponToEquip)
		{
			// 1. Get the Data Packet
			const FWeaponMovementData& WeaponData = WeaponToEquip->GetMovementData();

			// 2. Apply Physics to Character
			if (AFPSPlayerCharacter* FPSChar = Cast<AFPSPlayerCharacter>(GetOwner()))
			{
				// Server applies speed limits (Anti-Cheat / Logic)
				FPSChar->UpdateMovementSettings(WeaponData);
				FPSChar->SetOverlayState(WeaponData.OverlayState);
			}
		}
		
		// Server broadcasts delegate locally (for logic running on server)
		OnWeaponEquippedDelegate.Broadcast(EquippedWeapon);
	}
}

void UFPSCombatComponent::Server_EquipWeapon_Implementation(AFPSWeapon* WeaponToEquip)
{
	// The Server receives the Client's request and executes the real logic.
	EquipWeapon(WeaponToEquip);
}

// --- REPLICATION NOTIFICATION (Client Side) ---
void UFPSCombatComponent::OnRep_EquippedWeapon(AFPSWeapon* LastEquippedWeapon)
{
	// --- CLIENT VISUALS ---
	// This runs when the variable update arrives from the Server.
	// The Server knows the gun is attached, but the Client's engine doesn't know WHERE to put it yet.
	// This is where we handle the JITTER FIX.
	
	if (!IsValid(EquippedWeapon)) return;
	
	// 1. Update Weapon Physics locally (Client Visuals)
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	
	// 2. Attach Visuals (Client Side)
	// Snap the mesh to the hand socket.
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		EquippedWeapon->AttachToComponent(
			OwnerCharacter->GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale, 
			EquippedWeapon->GetWeaponHandSocketName()
		);
		
		EquippedWeapon->SetOwner(OwnerCharacter);
		
		// --- 3. JITTER FIX (Prediction) ---
		// We must apply the MaxWalkSpeed update on the Client immediately.
		// If we don't, the Client tries to move at 600 speed, Server forces 300, causing rubber-banding.
		if (AFPSPlayerCharacter* FPSChar = Cast<AFPSPlayerCharacter>(OwnerCharacter))
		{
			const FWeaponMovementData& WeaponData = EquippedWeapon->GetMovementData();
            
			// Sync Movement Speed
			FPSChar->UpdateMovementSettings(WeaponData); 
            
			// Sync Animation Pose
			FPSChar->SetOverlayState(WeaponData.OverlayState); 
		}
        
		// 4. Notify AnimInstance to run CalculateHandTransforms!
		OnWeaponEquippedDelegate.Broadcast(EquippedWeapon);
	}
}

void UFPSCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	// Get Viewport Size
	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	
	// Crosshair is center of screen
	const FVector2D CrosshairLocation(ViewportSize.X * 0.5f, ViewportSize.Y * 0.5f); // ViewportSize.X / 2.f, ViewportSize.Y / 2.f
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	const APlayerController* OwnerPlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!OwnerPlayerController) return;
	
	// Project 2D Screen to 3D World
	const bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		OwnerPlayerController,
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);
	
	if (bScreenToWorld)
	{
		// Start trace slightly in front of camera to avoid hitting self
		FVector TraceStart = CrosshairWorldPosition;
		FVector TraceEnd = CrosshairWorldDirection * EquippedWeapon->GetWeaponData()->Range;
		
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			TraceStart, 
			TraceEnd,
			ECC_Visibility
		);
		
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = TraceEnd;
			TraceHitResult.TraceEnd = TraceEnd; // Ensure we always have a valid target point
		}
	}
}

void UFPSCombatComponent::StartFire()
{
	if (!EquippedWeapon) return;
    
	bFireButtonPressed = true;

	// Try to fire the first shot immediately
	Fire();
}

void UFPSCombatComponent::StopFire()
{
	bFireButtonPressed = false;
	GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
}

void UFPSCombatComponent::Fire()
{
	// 1. CHECKS
	if (!bFireButtonPressed) return;
	if (!EquippedWeapon) return;
    
	// Use your new State Enum!
	if (CombatState != ECombatState::ECS_Unoccupied) return; 
    
	// Check Ammo
	if (EquippedWeapon->GetCurrentClipAmmo() <= 0) 
	{
		// Out of ammo? Stop the timer. 
		// Later we can auto-reload here.
		StopFire(); 
		return;
	}

	// 2. TRACE (Find Target)
	FHitResult HitResult;
	TraceUnderCrosshairs(HitResult);
    
	// If we hit nothing (sky), aim at the end of the trace
	FVector HitTarget = HitResult.bBlockingHit ? HitResult.ImpactPoint : HitResult.TraceEnd;

	// 3. SHOOT
	EquippedWeapon->Fire(HitTarget);

	// 4. AUTOMATIC FIRE LOOP
	if (EquippedWeapon->IsAutomatic())
	{
		GetWorld()->GetTimerManager().SetTimer(
			FireTimerHandle, 
			this, 
			&ThisClass::Fire, 
			EquippedWeapon->GetFireDelay(), 
			false // Not looping, we re-call it recursively to maintain control
		);
	}
}

void UFPSCombatComponent::Reload()
{
	// 1. Safety Checks
	if (!EquippedWeapon) return;
    
	// 2. State Check: Don't reload if already reloading or busy
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	// 3. Ammo Check: Don't reload if mag is full
	if (EquippedWeapon->GetCurrentClipAmmo() >= EquippedWeapon->GetMaxClipAmmo()) return;

	// 4. Backpack Check: Don't reload if we have no bullets left
	if (CarriedAmmo <= 0) return;

	// 5. Proceed
	Server_Reload();
}

void UFPSCombatComponent::Server_Reload_Implementation()
{
	// 1. Set Component State (The Brain)
	CombatState = ECombatState::ECS_Reloading;
	if (EquippedWeapon && EquippedWeapon->GetReloadMontage())
	{
		Multicast_Reload();
	}
}

void UFPSCombatComponent::Multicast_Reload_Implementation()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter && EquippedWeapon->GetReloadMontage())
	{
		OwnerCharacter->GetMesh()->GetAnimInstance()->Montage_Play(EquippedWeapon->GetReloadMontage());
	}
}

void UFPSCombatComponent::FinishReloading()
{
	// 1. AUTHORITY CHECK (The Gatekeeper)
	// If we are a Client, STOP. Do not touch the ammo.
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    
	// 2. STATE CHECK
	// If we aren't actually reloading (maybe we got stunned?), stop.
	if (CombatState != ECombatState::ECS_Reloading) return;
    
	if (!EquippedWeapon) return;

	// 3. THE MATH (Server Only)
	// Calculate how much space is in the mag
	int32 RoomInMag = EquippedWeapon->GetMaxClipAmmo() - EquippedWeapon->GetCurrentClipAmmo();
    
	// Calculate how much we can actually give
	int32 AmountToReload = FMath::Min(RoomInMag, CarriedAmmo);

	// Update the Replicated Variables
	EquippedWeapon->AddAmmo(AmountToReload);
	CarriedAmmo -= AmountToReload; // This variable needs to be Replicated too!
    
	// Reset State
	CombatState = ECombatState::ECS_Unoccupied;
}

void UFPSCombatComponent::OnRep_CombatState(ECombatState PreviousCombatState)
{
	const FString Prev = CombatStateToString(PreviousCombatState);
	const FString Curr = CombatStateToString(CombatState);

	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Red,
		FString::Printf(TEXT("Combat State Changed: %s -> %s"), *Prev, *Curr)
	);
}