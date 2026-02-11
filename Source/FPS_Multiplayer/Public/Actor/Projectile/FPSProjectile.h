// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class FPS_MULTIPLAYER_API AFPSProjectile : public AActor
{
	GENERATED_BODY()

public:

	AFPSProjectile();

protected:

	virtual void BeginPlay() override;
	
	/** * Called when the collision sphere hits something.
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
	
	// Assign in Blueprint (e.g., 20.0f)
	// It must match hit scans damage amount. If we are to have different types of projectiles that do various amounts of damage, hit scan logic must follow it.
	UPROPERTY(EditDefaultsOnly)
	float Damage = 20.f;
};
