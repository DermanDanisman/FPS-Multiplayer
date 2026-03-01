// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "Component/FPSCombatComponent.h"

#include "Actor/Weapon/FPSWeapon.h"
#include "Character/FPSPlayerCharacter.h"
#include "Component/FPSCharacterMovementComponent.h"
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
        
    AFPSPlayerCharacter* OwnerCharacter = Cast<AFPSPlayerCharacter>(GetOwner());
    if (!OwnerCharacter) return;
    
    // =========================================================
    // 1. LOCAL PREDICTION (Instant feedback)
    // =========================================================
    if (OwnerCharacter->IsLocallyControlled())
    {
        // CRITICAL FIX: We set this locally IMMEDIATELY. 
        // Now, when the AnimNotify fires, EquippedWeapon is valid!
        EquippedWeapon = WeaponToEquip;
        EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
        
        if (OwnerCharacter->Implements<UFPSWeaponHandlerInterface>())
        {
            if (IFPSWeaponHandlerInterface* WeaponHandler = Cast<IFPSWeaponHandlerInterface>(OwnerCharacter))
            {
                WeaponHandler->PlayEquipMontage(WeaponToEquip->GetEquipMontage());
            }
        }
    }

    // =========================================================
    // 2. NETWORK ROUTING
    // =========================================================
    if (!OwnerCharacter->HasAuthority())
    {
       Server_EquipWeapon(WeaponToEquip);
       return; 
    }
    
    // =========================================================
    // 3. SERVER AUTHORITATIVE LOGIC
    // =========================================================
    if (EquippedWeapon)
    {
       EquippedWeapon->OnAmmoChanged.Unbind();
    }
    
    EquippedWeapon = WeaponToEquip;
    EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
    EquippedWeapon->SetInitialClipAmmo();
    EquippedWeapon->SetOwner(OwnerCharacter);
    
    // CRITICAL FIX: Only play the montage on the Server if the Server is NOT the local player.
    // This prevents the Listen Server host from playing the montage twice.
    if (!OwnerCharacter->IsLocallyControlled())
    {
        if (OwnerCharacter->Implements<UFPSWeaponHandlerInterface>())
        {
           if (IFPSWeaponHandlerInterface* WeaponHandler = Cast<IFPSWeaponHandlerInterface>(OwnerCharacter))
           {
              WeaponHandler->PlayEquipMontage(EquippedWeapon->GetEquipMontage());
           }
        }
    }
}

void UFPSCombatComponent::Server_EquipWeapon_Implementation(AFPSWeapon* WeaponToEquip)
{
	// The Server receives the Client's request and executes the real logic.
	EquipWeapon(WeaponToEquip);
}

void UFPSCombatComponent::OnRep_EquippedWeapon(AFPSWeapon* LastEquippedWeapon)
{
	if (LastEquippedWeapon)
	{
		LastEquippedWeapon->OnAmmoChanged.Unbind();
	}
    
	if (!IsValid(EquippedWeapon)) return;
    
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
    
	AFPSPlayerCharacter* OwnerCharacter = Cast<AFPSPlayerCharacter>(GetOwner());
	if (OwnerCharacter)
	{
		// --- SIMULATED PROXIES ---
		if (!OwnerCharacter->IsLocallyControlled())
		{
			// Notice we DO NOT attach the mesh or link layers yet!
			// We ONLY play the montage. The AnimNotify will handle the rest.
			if (OwnerCharacter->Implements<UFPSWeaponHandlerInterface>())
			{
				if (IFPSWeaponHandlerInterface* WeaponHandler = Cast<IFPSWeaponHandlerInterface>(OwnerCharacter))
				{
					WeaponHandler->PlayEquipMontage(EquippedWeapon->GetEquipMontage());
				}
			}
		}
       
		EquippedWeapon->SetOwner(OwnerCharacter);
       
		// Force sync movement speeds for prediction
		const FWeaponMovementData& WeaponData = EquippedWeapon->GetMovementData();
		OwnerCharacter->GetFPSMovementComponent()->UpdateMovementSettings(WeaponData); 
		OwnerCharacter->SetOverlayState(WeaponData.OverlayState); 
	}
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
	ShotsFired = 0;
	
	// FIX: Check if weapon exists before accessing it!
	if (EquippedWeapon) 
	{
		EquippedWeapon->ResetRecoil();
	}
	
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
		OnWeaponAmmoChanged.Broadcast(EquippedWeapon->GetCurrentClipAmmo(), EquippedWeapon->GetMaxClipAmmo());
		ShotsFired++;
		EquippedWeapon->ApplyRecoil(ShotsFired);

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
	AFPSPlayerCharacter* OwnerCharacter = Cast<AFPSPlayerCharacter>(GetOwner());
	if (OwnerCharacter)
	{
		if (EquippedWeapon && EquippedWeapon->GetReloadMontage())
		{
			// Play on the correct mesh!
			USkeletalMeshComponent* TargetMesh = OwnerCharacter->IsLocallyControlled() ? OwnerCharacter->GetLocalMesh() : OwnerCharacter->GetMesh();
			TargetMesh->GetAnimInstance()->Montage_Play(EquippedWeapon->GetReloadMontage());
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
	
	// 5. UPDATE UI (Manual Broadcast)
	// Do NOT call MonitorWeapon() here.
	OnWeaponAmmoChanged.Broadcast(EquippedWeapon->GetCurrentClipAmmo(), EquippedWeapon->GetMaxClipAmmo());
	OnCarriedAmmoChanged.Broadcast(CarriedAmmo);
}

void UFPSCombatComponent::FinishWeaponEquip()
{
	if (!IsValid(EquippedWeapon)) return;
    
	AFPSPlayerCharacter* OwnerCharacter = Cast<AFPSPlayerCharacter>(GetOwner());
	if (!OwnerCharacter) return;

	// 1. Target the correct mesh
	USkeletalMeshComponent* TargetMesh = OwnerCharacter->IsLocallyControlled() ? OwnerCharacter->GetLocalMesh() : OwnerCharacter->GetMesh();

	// 2. Attach the Visuals
	EquippedWeapon->AttachToComponent(
		TargetMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale, 
		EquippedWeapon->GetWeaponHandSocketName()
	);
    
	// 3. Link the Animation Layers to the targeted mesh
	TargetMesh->LinkAnimClassLayers(EquippedWeapon->GetEquippedAnimInstanceClass());
    
	// 4. Fire the Broadcasts
	MonitorWeapon(EquippedWeapon);
	OnWeaponEquipped.Broadcast(EquippedWeapon);
}

// Helper function to keep code clean
void UFPSCombatComponent::MonitorWeapon(AFPSWeapon* Weapon)
{
	if (!Weapon) return;

	// Unbind previous weapon if necessary (Optional optimization)
    
	// Bind to the new weapon
	Weapon->OnAmmoChanged.BindLambda([this](int32 Current, int32 Max)
	{
		// FORWARD the message to the UI
		OnWeaponAmmoChanged.Broadcast(Current, Max);
	});
    
	// Broadcast initial state immediately so UI syncs up
	OnWeaponAmmoChanged.Broadcast(Weapon->GetCurrentClipAmmo(), Weapon->GetMaxClipAmmo());
}