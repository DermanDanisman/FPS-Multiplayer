// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#include "Actor/Weapon/FPSWeapon.h"

#include "Character/FPSPlayerCharacter.h"
#include "Component/FPSCombatComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"

AFPSWeapon::AFPSWeapon()
{
	// Optimization: Weapons usually don't need to run logic every frame.
	PrimaryActorTick.bCanEverTick = false;
	
	// MULTIPLAYER ESSENTIAL:
	// This tells the engine: "If the Server spawns this, spawn a copy on all Clients too."
	// Without this, clients wouldn't see the gun.
	bReplicates = true;
	
	// Ensures that if a client joins late, they still download this actor's data.
	bNetLoadOnClient = true;

	// --- 1. Construct the Mesh ---
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>("WeaponMesh");
	SetRootComponent(WeaponMesh); // The gun mesh controls the actor's location

	// --- 2. Configure Physics (Initial State) ---
	// Block all channels (Walls, Floor) so it doesn't fall through the world.
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
	// Ignore the Pawn so players don't "trip" over the gun while walking near it.
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	// Start with NoCollision until we explicitly enable it (safety first).
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// --- 3. Construct the UI ---
	InteractionWidget = CreateDefaultSubobject<UWidgetComponent>("InteractionWidget");
	InteractionWidget->SetupAttachment(GetRootComponent());
}

void AFPSWeapon::Interact_Implementation(APawn* InstigatorPawn)
{
	AFPSPlayerCharacter* FPSPlayerCharacter = Cast<AFPSPlayerCharacter>(InstigatorPawn);
	if (!IsValid(FPSPlayerCharacter)) return;
	
	FPSPlayerCharacter->GetCombatComponent()->EquipWeapon(this);
}

void AFPSWeapon::OnFocusGained_Implementation(APawn* InstigatorPawn)
{
	if (InteractionWidget)
	{
		InteractionWidget->SetVisibility(true);
	}
}

void AFPSWeapon::OnFocusLost_Implementation(APawn* InstigatorPawn)
{
	if (InteractionWidget)
	{
		InteractionWidget->SetVisibility(false);
	}
}

void AFPSWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	// Ensure the widget is hidden by default when the game starts.
	if (IsValid(InteractionWidget)) 
	{
		InteractionWidget->SetVisibility(false);
	}
}

void AFPSWeapon::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// Register the variable for replication
	DOREPLIFETIME(AFPSWeapon, WeaponState);
}

void AFPSWeapon::SetWeaponState(const EWeaponState NewWeaponState)
{
	WeaponState = NewWeaponState;
	OnRep_WeaponState(); // Force the visual/physics update immediately on Server
}

void AFPSWeapon::OnRep_WeaponState()
{
	// This runs on the Client automatically when the state changes.
	// We ALSO call this manually on the Server to ensure they match!
	
	switch (WeaponState)
	{
		case EWeaponState::EWS_Equipped:
			// 1. Disable Physics (So it doesn't fall out of hand)
			WeaponMesh->SetSimulatePhysics(false);
			WeaponMesh->SetEnableGravity(false);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	            
			// 3. Hide Widget (Just in case)
			if (InteractionWidget) InteractionWidget->SetVisibility(false);
			break;

		case EWeaponState::EWS_Dropped:
			// 1. Enable Physics (So it falls to the floor)
			WeaponMesh->SetSimulatePhysics(true);
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

			break;
		default: ;
	}
}
