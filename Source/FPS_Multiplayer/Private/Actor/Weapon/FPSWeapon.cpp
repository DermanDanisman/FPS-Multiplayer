// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#include "Actor/Weapon/FPSWeapon.h"
#include "Character/FPSPlayerCharacter.h"
#include "Component/FPSCombatComponent.h"
#include "Components/SphereComponent.h"
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

	// --- 3. Construct the Pickup Area ---
	AreaSphere = CreateDefaultSubobject<USphereComponent>("AreaSphere");
	AreaSphere->SetupAttachment(GetRootComponent());
	
	// CRITICAL MULTIPLAYER LOGIC:
	// We disable collision in the constructor.
	// We will ONLY enable it inside BeginPlay() IF we are the Server.
	// This ensures Clients cannot generate "Fake" overlap events locally.
	AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// --- 3. Construct the UI ---
	InteractionWidget = CreateDefaultSubobject<UWidgetComponent>("InteractionWidget");
	InteractionWidget->SetupAttachment(GetRootComponent());
}

void AFPSWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	// --- SERVER AUTHORITY CHECK ---
	// "HasAuthority()" asks: "Is this the Master Copy of the actor on the Server?"
	// True = Server | False = Client (Proxy)
	if (HasAuthority()) 
	{
		// Only the Server should detect Overlaps.
		// If a Client overlaps, it does nothing locally. The Server must tell them "You picked it up."
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    
		// Specifically look for Pawns (Players) to trigger the pickup.
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		
		// 3. Bind the Events
		// These functions will ONLY fire on the Server.
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnAreaSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnAreaSphereEndOverlap);
	}
	
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

void AFPSWeapon::OnAreaSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// CASTING: Check if the thing that touched us is actually a Player Character.
	AFPSPlayerCharacter* FPSPlayerCharacter = Cast<AFPSPlayerCharacter>(OtherActor);
	if (!IsValid(FPSPlayerCharacter)) return;
	
	// Hand off logic to the Character.
	// We do NOT set visibility here, because that would show the widget to EVERYONE (including the Server host).
	// The Character class will decide who sees the widget based on "IsLocallyControlled".
	FPSPlayerCharacter->GetCombatComponent()->SetOverlappingWeapon(this);
}

void AFPSWeapon::OnAreaSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AFPSPlayerCharacter* FPSPlayerCharacter = Cast<AFPSPlayerCharacter>(OtherActor);
	if (!IsValid(FPSPlayerCharacter)) return;
	
	// Tell the character they left the weapon zone.
	FPSPlayerCharacter->GetCombatComponent()->SetOverlappingWeapon(nullptr);
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

			// 2. Disable Pickup Sphere (So we can't trigger overlaps while holding it)
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	            
			// 3. Hide Widget (Just in case)
			if (InteractionWidget) InteractionWidget->SetVisibility(false);
			break;

		case EWeaponState::EWS_Dropped:
			// 1. Enable Physics (So it falls to the floor)
			WeaponMesh->SetSimulatePhysics(true);
			WeaponMesh->SetEnableGravity(true);
			WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	            
			// 2. Enable Pickup Sphere
			AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			AreaSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
			break;
		default: ;
	}
}
