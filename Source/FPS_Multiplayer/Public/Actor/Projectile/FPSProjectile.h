// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

/**
 * @class AFPSProjectile
 * @brief Base class for physical projectiles (Rockets, slow-moving bullets).
 * Relies on the spawning weapon to initialize its damage and properties.
 */
UCLASS()
class FPS_MULTIPLAYER_API AFPSProjectile : public AActor
{
	GENERATED_BODY()

public:

	AFPSProjectile();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	/**
	 * Called by the Weapon immediately after spawning the projectile.
	 * Passes necessary combat stats to the bullet.
	 */
	void InitializeProjectile(float InDamage);

protected:

	virtual void BeginPlay() override;
	
	/** 
	 * Called when the collision sphere hits something.
	 * Only runs on the Server if bReplicates is true and movement is replicated.
	 */
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> CollisionSphere;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
	
	// --- STATE ---
    
	/** The damage this projectile will deal upon impact. Set by the Weapon. */
	UPROPERTY(Replicated)
	float Damage = 0.f;
};
