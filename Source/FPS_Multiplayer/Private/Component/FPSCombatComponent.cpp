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

// =========================================================================
//                        WEAPON HANDLING
// =========================================================================

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

// =========================================================================
//                        FIRING LOGIC
// =========================================================================

void UFPSCombatComponent::StartFire()
{
	if (!EquippedWeapon) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	
	bFireButtonPressed = true;
	CombatState = ECombatState::ECS_Firing;
	Fire(); // Fire first shot immediately
}

void UFPSCombatComponent::StopFire()
{
	bFireButtonPressed = false;
	CombatState = ECombatState::ECS_Unoccupied;
	GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
}

void UFPSCombatComponent::Fire()
{
	// 1. GATES
	if (!bFireButtonPressed) return;
	if (!EquippedWeapon) return;

	if (CombatState == ECombatState::ECS_Firing)
	{
		// 2. AMMO CHECK
		if (EquippedWeapon->GetCurrentClipAmmo() <= 0) 
		{
			// Empty Click Sound could go here
			StopFire(); 
			// Optional: Auto-Reload
			// Reload(); 
			return;
		}

		// 3. EXECUTION
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
    
		FVector HitTarget = HitResult.bBlockingHit ? HitResult.ImpactPoint : HitResult.TraceEnd;
		EquippedWeapon->Fire(HitTarget);

		// 4. AUTOMATIC LOOP
		if (EquippedWeapon->IsAutomatic())
		{
			GetWorld()->GetTimerManager().SetTimer(
			   FireTimerHandle, 
			   this, 
			   &ThisClass::Fire, 
			   EquippedWeapon->GetFireDelay(), 
			   false 
			);
		}
	}
	else
	{
		StopFire(); 
	}
}

void UFPSCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	// Safe Viewport Access
	FVector2D ViewportSize = FVector2D::ZeroVector;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	
	// Center of Screen
	const FVector2D CrosshairLocation(ViewportSize.X * 0.5f, ViewportSize.Y * 0.5f); // ViewportSize.X / 2.f, ViewportSize.Y / 2.f
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	if (APlayerController* PC = Cast<APlayerController>(Cast<ACharacter>(GetOwner())->GetController()))
	{
		bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		   PC,
		   CrosshairLocation,
		   CrosshairWorldPosition,
		   CrosshairWorldDirection
		);
        
		if (bScreenToWorld)
		{
			// Start slightly forward to avoid colliding with own capsule if running fast
			FVector Start = CrosshairWorldPosition + (CrosshairWorldDirection * 30.f); 
			FVector End = Start + (CrosshairWorldDirection * EquippedWeapon->GetWeaponRange());
           
			GetWorld()->LineTraceSingleByChannel(
			   TraceHitResult,
			   Start, 
			   End,
			   ECC_Visibility
			);
           
			if (!TraceHitResult.bBlockingHit)
			{
				TraceHitResult.ImpactPoint = End;
				TraceHitResult.TraceEnd = End;
			}
		}
	}
}

// =========================================================================
//                        RELOAD LOGIC
// =========================================================================

void UFPSCombatComponent::Reload()
{
	// Client Side Checks (Save Bandwidth)
	// 1. Safety & State Checks
	if (!EquippedWeapon) return;
	if (CombatState != ECombatState::ECS_Unoccupied) return;
    
	// 2. Ammo Checks
	// Don't reload if full
	if (EquippedWeapon->GetCurrentClipAmmo() >= EquippedWeapon->GetMaxClipAmmo()) return;
	// Don't reload if empty reserve
	if (CarriedAmmo <= 0) return;

	// 3. Execute
	Server_Reload();
}

void UFPSCombatComponent::Server_Reload_Implementation()
{
	// Server Side Checks (Safety)
	if (!EquippedWeapon) return;
    
	CombatState = ECombatState::ECS_Reloading;
    
	// Trigger Visuals for Everyone
	if (EquippedWeapon->GetReloadMontage())
	{
		Multicast_Reload();
	}
}

void UFPSCombatComponent::Multicast_Reload_Implementation()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		if (EquippedWeapon && EquippedWeapon->GetReloadMontage())
		{
			OwnerCharacter->GetMesh()->GetAnimInstance()->Montage_Play(EquippedWeapon->GetReloadMontage());
		}
	}
}

void UFPSCombatComponent::FinishReloading()
{
	// 1. AUTHORITY CHECK (The Gatekeeper)
	// If we are a Client, STOP. Do not touch the ammo.
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    
	// 2. STATE CHECK
	// If we aren't actually reloading (maybe we got stunned?), stop.
	// State Guard (Prevent cheating by calling FinishReloading without starting it)
	if (CombatState != ECombatState::ECS_Reloading) return;
	if (!EquippedWeapon) return;
    
	if (!EquippedWeapon) return;

	// 3. --- MATH --- (Server Only)
	// Calculate how much space is in the mag
	int32 RoomInMag = EquippedWeapon->GetMaxClipAmmo() - EquippedWeapon->GetCurrentClipAmmo();
    
	// Calculate how much we can actually give
	int32 AmountToReload = FMath::Min(RoomInMag, CarriedAmmo);

	// Update the Replicated Variables
	EquippedWeapon->AddAmmo(AmountToReload);
	CarriedAmmo -= AmountToReload; // This variable needs to be Replicated too!
    
	// 4. --- RESET ---
	CombatState = ECombatState::ECS_Unoccupied;
}

void UFPSCombatComponent::OnRep_CombatState(ECombatState PreviousCombatState)
{
	// Optional: Use this to update HUD state (Crosshair spread, etc.)
	
	const FString Prev = CombatStateToString(PreviousCombatState);
	const FString Curr = CombatStateToString(CombatState);

	GEngine->AddOnScreenDebugMessage(
		-1,
		5.f,
		FColor::Red,
		FString::Printf(TEXT("Combat State Changed: %s -> %s"), *Prev, *Curr)
	);
}