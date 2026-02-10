// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#include "Actor/Weapon/FPSWeapon.h"

#include "Character/FPSPlayerCharacter.h"
#include "Component/FPSCombatComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
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



void AFPSWeapon::Fire(const FVector& HitTarget)
{
	// 1. DEDUCT AMMO (Visual Prediction)
	// We clamp it so UI updates instantly. Real authority is on Server.
	CurrentClipAmmo = FMath::Clamp(CurrentClipAmmo - 1, 0, GetMaxClipAmmo());
	
	// 2. PLAY FX (Visual Prediction)
	// Play sound/flash immediately on the client firing so it feels "Lag Free"
	PlayFireEffects(HitTarget);
	
	// 3. TELL SERVER
	Server_Fire(HitTarget);
}

void AFPSWeapon::Server_Fire_Implementation(const FVector& TraceHitTarget)
{
	// 1. VALIDATION (Anti-Cheat)
	// If client lied about ammo, we stop them here.
	if (CurrentClipAmmo < 0) return;
	
	// Deduct Actual Ammo
	CurrentClipAmmo = FMath::Clamp(CurrentClipAmmo - 1, 0, GetMaxClipAmmo());
	
	// 2. DEAL DAMAGE
	// Only the Server can deal damage.
	// We assume the TraceHitTarget is valid for now (in a real anti-cheat, we'd re-trace here).
	/* UGameplayStatics::ApplyPointDamage(
	   TargetActor, Damage, ...
	);
	*/

	// 3. REPLICATE FX
	// Tell all other clients to play the sound/flash
	Multicast_Fire(TraceHitTarget);
}

void AFPSWeapon::Multicast_Fire_Implementation(const FVector& TraceHitTarget)
{
	// Optimization: The owner (you) already played the effect in Fire().
	// We don't want to play it twice (echo effect).
	if (!GetOwner()) return;
	
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled()) return;

	PlayFireEffects(TraceHitTarget);
}

void AFPSWeapon::PlayFireEffects(const FVector& TraceHitTarget) const
{
	if (!WeaponData) return;

	// Muzzle Flash
	if (WeaponData->MuzzleFlash)
	{
		UGameplayStatics::SpawnEmitterAttached(
			WeaponData->MuzzleFlash, 
			WeaponMesh, 
			WeaponData->MuzzleSocketName,
			FVector::ZeroVector, 
			FRotator::ZeroRotator, 
			EAttachLocation::SnapToTarget, 
			true
		);
	}

	// Fire Sound
	if (WeaponData->FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WeaponData->FireSound, GetActorLocation());
	}
    
	// Impact Particles (Optional)
	if (WeaponData->ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), WeaponData->ImpactParticles, TraceHitTarget);
	}
	
	UAnimMontage* MontageToPlay = WeaponData->HipFireMontage;

	// Check if character is Aiming
	if (ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		// You need to cast to your specific character to read the Aim State
		if (AFPSPlayerCharacter* FPSChar = Cast<AFPSPlayerCharacter>(Char))
		{
			if (FPSChar->GetLayerStates().AimState == EAimState::EAS_ADS)
			{
				MontageToPlay = WeaponData->AimedFireMontage;
			}
		}
        
		// Play the selected montage
		if (MontageToPlay)
		{
			Char->GetMesh()->GetAnimInstance()->Montage_Play(MontageToPlay);
		}
	}
	
	if (WeaponData->FireCameraShake)
	{
		ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
		if (OwnerCharacter->IsLocallyControlled())
		{
			if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
			{
				PC->ClientStartCameraShake(WeaponData->FireCameraShake, 1.0f);
			}
		}
	}
}

void AFPSWeapon::AddAmmo(int32 AmountToAdd)
{
	CurrentClipAmmo = FMath::Clamp(CurrentClipAmmo + AmountToAdd, 0, GetMaxClipAmmo());
}