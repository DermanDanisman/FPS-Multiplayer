// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "Actor/Projectile/FPSProjectile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"


AFPSProjectile::AFPSProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
    
	// Networking
	bReplicates = true;
	SetReplicatingMovement(true);
    
	// 1. Collision (Root)
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(5.0f);
	CollisionSphere->SetCollisionProfileName("Projectile");
	// Optimization: Projectiles shouldn't return overlap events unless we specifically need them
	CollisionSphere->bReturnMaterialOnMove = true; // Useful for impact FX later
	SetRootComponent(CollisionSphere);
    
	// 2. Visuals
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionSphere);
	ProjectileMesh->SetCollisionProfileName("NoCollision");
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
	// 3. Movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovement->InitialSpeed = 15000.f;
	ProjectileMovement->MaxSpeed = 15000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 1.0f; // Enable bullet drop!
}

void AFPSProjectile::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, Damage);
}

void AFPSProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	// Only the Server needs to listen for hits to deal damage
	if (HasAuthority())
	{
		CollisionSphere->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	}
    
	// Safety fallback: Destroy projectile after 5 seconds if it flies into the void
	SetLifeSpan(5.f);
}

void AFPSProjectile::InitializeProjectile(float InDamage)
{
	// The Weapon calls this right after spawning
	Damage = InDamage;
}

void AFPSProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	// 1. Authority Guard
	if (!HasAuthority()) return;

	// 2. Self-Hit Guard (Don't shoot ourselves or our gun)
	if (OtherActor == GetOwner() || OtherActor == GetInstigator()) return;
    
	// 3. Deal Damage
	if (OtherActor)
	{
		UGameplayStatics::ApplyPointDamage(
		   OtherActor,
		   Damage,
		   GetActorForwardVector(),
		   Hit,
		   GetInstigatorController(),
		   this,
		   UDamageType::StaticClass()
		);
	}
	
	// 4. Cleanup
	Destroy();
}


