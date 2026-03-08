// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#include "Actor/Weapon/FPSWeapon.h"

#include "Actor/Projectile/FPSProjectile.h"
#include "Component/FPSRecoilComponent.h"
#include "Components/WidgetComponent.h"
#include "Curves/CurveVector.h"
#include "GameFramework/Character.h"
#include "Interface/FPSWeaponHandlerInterface.h"
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
	
	RecoilComponent = CreateDefaultSubobject<UFPSRecoilComponent>("RecoilComponent");
	
	// --- 3. Construct the UI ---
	InteractionWidget = CreateDefaultSubobject<UWidgetComponent>("InteractionWidget");
	InteractionWidget->SetupAttachment(GetRootComponent());
}

void AFPSWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	if (WeaponData->WeaponMesh)
		WeaponMesh->SetSkeletalMesh(WeaponData->WeaponMesh);
	
	// Ensure the widget is hidden by default when the game starts.
	if (IsValid(InteractionWidget)) 
	{
		InteractionWidget->SetVisibility(false);
	}
}

void AFPSWeapon::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AFPSWeapon, WeaponState);
	DOREPLIFETIME(AFPSWeapon, CurrentClipAmmo);
}

// =========================================================================
//                        STATE MANAGEMENT
// =========================================================================

void AFPSWeapon::SetWeaponState(const EWeaponState NewWeaponState)
{
	WeaponState = NewWeaponState;
	// We call this manually on the Server so physics update immediately before replication happens
	OnRep_WeaponState();
}

void AFPSWeapon::OnRep_WeaponState()
{
	// This runs on the Client automatically when the state changes.
	// We ALSO call this manually on the Server to ensure they match!
	
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		// 1. Disable Physics (So it doesn't fall out of hand)
		GetWeaponMesh()->SetSimulatePhysics(false);
		GetWeaponMesh()->SetEnableGravity(false);
		GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	            
		// 3. Hide Widget (Just in case)
		if (InteractionWidget) InteractionWidget->SetVisibility(false);
		break;

	case EWeaponState::EWS_Dropped:
		// 1. Enable Physics (So it falls to the floor)
		GetWeaponMesh()->SetSimulatePhysics(true);
		GetWeaponMesh()->SetEnableGravity(true);
		GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
		
	case EWeaponState::EWS_Initial:
		// Logic for spawning on map (e.g., enable a trigger box)
		break;
	default: ;
	}
}

// =========================================================================
//                        COMBAT LOGIC
// =========================================================================

void AFPSWeapon::Fire(const FVector& HitTarget)
{
	// 1. PREDICTION (Client Only)
	// If we are the Server, we skip this because Server_Fire will handle it immediately.
	// If we are a Client, we do this to update the UI instantly (Prediction).
	if (CurrentClipAmmo > 0 && !HasAuthority()) 
	{
		CurrentClipAmmo = FMath::Clamp(CurrentClipAmmo - 1, 0, WeaponData->MaxClipAmmo);
	}

	// 2. VISUALS (Local)
	PlayFireEffects(HitTarget);
    
	// 3. AUTHORITY (Network)
	Server_Fire(HitTarget);
}

void AFPSWeapon::Server_Fire_Implementation(const FVector& TraceHitTarget)
{
	// 1. VALIDATION
	if (CurrentClipAmmo <= 0 || !WeaponData) return;
    
	// Deduct Actual Ammo (Authoritative)
	CurrentClipAmmo = FMath::Clamp(CurrentClipAmmo - 1, 0, WeaponData->MaxClipAmmo);
	
	// --- 2. BALLISTICS FORK ---
	switch (WeaponData->FireType)
	{
		case EWeaponFireType::EWFT_HitScan:
			{
				// --- OPTION A: HITSCAN ---
				// The bullet has already "arrived" at TraceHitTarget instantly.
	            
				// 1. Re-Verify the hit (Anti-Cheat / Latency Compensation would go here)
				// For now, we trust the input vector or perform a short sanity check trace.
	            
				// Calculate line from Muzzle to where the Camera looked
				FVector TraceStart = GetWeaponMesh()->GetSocketLocation(WeaponData->MuzzleSocketName);
				FHitResult HitResult;
				
				FCollisionQueryParams Params;
				Params.AddIgnoredActor(this);
				Params.AddIgnoredActor(GetOwner());
	            
				// Perform the authoritative damage trace
				bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceHitTarget, ECC_Visibility, Params);
	            
				if (bHit && HitResult.GetActor())
				{
					UGameplayStatics::ApplyPointDamage(
						HitResult.GetActor(),
						WeaponData->Damage,
						(TraceHitTarget - TraceStart).GetSafeNormal(),
						HitResult,
						GetOwner()->GetInstigatorController(),
						this,
						UDamageType::StaticClass()
					);
				}
				break;
			}

		case EWeaponFireType::EWFT_Projectile:
			{
				// --- OPTION B: PROJECTILE ---
				// Spawn a physical actor. It will handle its own collision and damage later.
	            
				FVector MuzzleLocation = GetWeaponMesh()->GetSocketLocation(WeaponData->MuzzleSocketName);
				FRotator TargetRotation;

				// Parallax Correction Math (The "Sideways Bullet" Fix)
				float DistanceToTarget = FVector::Dist(MuzzleLocation, TraceHitTarget);

				if (DistanceToTarget < 150.f) // If target is closer than 1.5 meters...
				{
					// Too close! Shoot straight out of the barrel to prevent sideways bullets.
					TargetRotation = GetWeaponMesh()->GetSocketRotation(WeaponData->MuzzleSocketName);
				}
				else
				{
					// Target is far. Triangulate normally.
					// Calculate Rotation: Look from Muzzle -> Crosshair Target
					TargetRotation = (TraceHitTarget - MuzzleLocation).Rotation();
				}

				if (WeaponData->ProjectileClass)
				{
					// 1. Begin Deferred Spawn (Returns pointer, but pauses execution)
					AFPSProjectile* Projectile = GetWorld()->SpawnActorDeferred<AFPSProjectile>(
						WeaponData->ProjectileClass,
						FTransform(TargetRotation, MuzzleLocation),
						GetOwner(),
						Cast<APawn>(GetOwner()),
						ESpawnActorCollisionHandlingMethod::AlwaysSpawn
					);
					
					if (IsValid(Projectile))
					{
						// 2. Inject Runtime Data safely
						Projectile->InitializeProjectile(WeaponData->Damage);
        
						// 3. Unpause and finalize the Actor
						UGameplayStatics::FinishSpawningActor(Projectile, FTransform(TargetRotation, MuzzleLocation));
					}
				}
				break;
			}
	}
	
	// Server needs to broadcast too so the Host UI updates
	OnAmmoChanged.ExecuteIfBound(CurrentClipAmmo, WeaponData->MaxClipAmmo);

	// --- 3. REPLICATE FX ---
	// Tell other clients to play sounds and flashes
	Multicast_Fire(TraceHitTarget);
}

void AFPSWeapon::Multicast_Fire_Implementation(const FVector& TraceHitTarget)
{
	// Optimization: The Autonomous Proxy (Shooter) already played FX in Fire().
	// We only want Simulated Proxies (Other Players) to execute this.
	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (OwnerCharacter->IsLocallyControlled()) return;
	}

	PlayFireEffects(TraceHitTarget);
}

void AFPSWeapon::PlayFireEffects(const FVector& TraceHitTarget) const
{
    if (!WeaponData) return;

    // 1. Muzzle Flash
    if (WeaponData->MuzzleFlash)
    {
        UGameplayStatics::SpawnEmitterAttached(
            WeaponData->MuzzleFlash, 
            GetWeaponMesh(), 
            WeaponData->MuzzleSocketName,
            FVector::ZeroVector, 
            FRotator::ZeroRotator, 
            EAttachLocation::SnapToTarget, 
            true
        );
    }

    // 2. Fire Sound
    if (WeaponData->FireSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, WeaponData->FireSound, GetActorLocation());
    }
    
    // 3. Impact Particles
    if (WeaponData->ImpactParticles)
    {
        // Simple impact logic. Ideally, trace material to decide WHICH particle to play.
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), WeaponData->ImpactParticles, TraceHitTarget);
    }
	
	// 4. Montages & Camera Shake
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter && OwnerCharacter->Implements<UFPSWeaponHandlerInterface>())
	{
		IFPSWeaponHandlerInterface* WeaponHandler = Cast<IFPSWeaponHandlerInterface>(OwnerCharacter);
		if (WeaponHandler)
		{
			WeaponHandler->PlayFireMontage(WeaponData->FireMontage, WeaponData->AimedFireMontage);
		}
	}
        
	// Apply Camera Shake (Local Player Only)
	if (OwnerCharacter->IsLocallyControlled() && WeaponData->FireCameraShake)
	{
		if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
		{
			PC->ClientStartCameraShake(WeaponData->FireCameraShake, 1.0f);
		}
	}
}

void AFPSWeapon::OnRep_CurrentClipAmmo()
{
	OnAmmoChanged.ExecuteIfBound(CurrentClipAmmo, WeaponData->MaxClipAmmo);
}

// =========================================================================
//                        HELPERS & INTERFACE
// =========================================================================

void AFPSWeapon::AddAmmo(int32 AmountToAdd)
{
	CurrentClipAmmo = FMath::Clamp(CurrentClipAmmo + AmountToAdd, 0, WeaponData->MaxClipAmmo);
}

void AFPSWeapon::SetInitialClipAmmo()
{
	CurrentClipAmmo = WeaponData->MaxClipAmmo;
	
}

void AFPSWeapon::ApplyRecoil(const int32 ShotsFired) const
{
	RecoilComponent->ApplyRecoil(
		WeaponData->RecoilPatternCurve->GetVectorValue(ShotsFired),
		WeaponData->RecoilInterpSpeed, 
		WeaponData->RecoilRecoverySpeed
	);
}

void AFPSWeapon::ResetRecoil() const
{
	RecoilComponent->StartRecovery();
}

void AFPSWeapon::Interact_Implementation(APawn* InstigatorPawn)
{
	// Check if the pawn implements the interface (works for Player AND AI)
	if (InstigatorPawn && InstigatorPawn->Implements<UFPSWeaponHandlerInterface>())
	{
		// Execute the Interface call
		// Note: For C++ interfaces, we execute via the I-class pointer.
		IFPSWeaponHandlerInterface* WeaponHandler = Cast<IFPSWeaponHandlerInterface>(InstigatorPawn);
		if (WeaponHandler)
		{
			WeaponHandler->SetCurrentWeapon(this);
		}
	}
}

void AFPSWeapon::OnFocusGained_Implementation(APawn* InstigatorPawn)
{
	if (InteractionWidget) InteractionWidget->SetVisibility(true);
}

void AFPSWeapon::OnFocusLost_Implementation(APawn* InstigatorPawn)
{
	if (InteractionWidget) InteractionWidget->SetVisibility(false);
}