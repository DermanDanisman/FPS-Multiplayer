// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "Actor/Projectile/FPSProjectile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"


AFPSProjectile::AFPSProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(true);
	
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetCollisionProfileName("Projectile");
	SetRootComponent(CollisionSphere);
	
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionSphere);
	ProjectileMesh->SetCollisionProfileName("NoCollision");
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->InitialSpeed = 15000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
}

void AFPSProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	// On Server
	if (HasAuthority())
	{
		CollisionSphere->OnComponentHit.AddDynamic(this, &AFPSProjectile::OnHit);
	}
	
	// SetLifeSpan(5.f);
}

void AFPSProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	// Server Authority Check
	if (!HasAuthority()) return;

	// Don't hit yourself
	if (OtherActor == GetOwner()) return;
	
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
	
	Destroy(); // Poof
}


