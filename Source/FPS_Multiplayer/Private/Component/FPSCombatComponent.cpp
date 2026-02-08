// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "Component/FPSCombatComponent.h"

#include "Actor/Weapon/FPSWeapon.h"
#include "Animation/FPSAnimInstance.h"
#include "Character/FPSPlayerCharacter.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"


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
			CharacterWeaponSocket
		);
		
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
			CharacterWeaponSocket
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

