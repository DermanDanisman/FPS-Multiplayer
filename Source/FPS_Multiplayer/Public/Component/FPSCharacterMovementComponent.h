// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/DataAsset/FPSWeaponData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FPSCharacterMovementComponent.generated.h"

/**
 * FLyraCharacterGroundInfo
 *
 *	Information about the ground under the character.  It only gets updated as needed.
 */
USTRUCT(BlueprintType)
struct FCharacterGroundInfo
{
	GENERATED_BODY()

	FCharacterGroundInfo()
		: LastUpdateFrame(0)
		, GroundDistance(0.0f)
	{}

	uint64 LastUpdateFrame;

	UPROPERTY(BlueprintReadOnly)
	FHitResult GroundHitResult;

	UPROPERTY(BlueprintReadOnly)
	float GroundDistance;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPS_MULTIPLAYER_API UFPSCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	
	UFPSCharacterMovementComponent();
	
	void SetReplicatedAcceleration(const FVector& InAcceleration);

	// Returns the current ground info.  Calling this will update the ground info if it's out of date.
	UFUNCTION(BlueprintCallable, Category = "FPSCharacterMovement")
	const FCharacterGroundInfo& GetGroundInfo();
	
	void UpdateMovementSettings(const FWeaponMovementData& NewData);
	
	UFUNCTION(BlueprintPure, Category = "FPSCharacterMovement")
	float GetWalkSpeed() const { return WalkSpeed; }
	
	UFUNCTION(BlueprintPure, Category = "FPSCharacterMovement")
	float GetSprintSpeed() const { return SprintSpeed; }
	
	UFUNCTION(BlueprintPure, Category = "FPSCharacterMovement")
	float GetRunSpeed() const { return RunSpeed; };

protected:

	virtual void InitializeComponent() override;
	
	// Cached ground info for the character.  Do not access this directly!  It's only updated when accessed via GetGroundInfo().
	FCharacterGroundInfo CachedGroundInfo;
	
	UPROPERTY(Transient)
	bool bHasReplicatedAcceleration = false;
	
	// A safe place to store network acceleration before the engine tries to overwrite it
	UPROPERTY(Transient)
	FVector SafeReplicatedAcceleration;
	
	// Cache these values from UpdateMovementSettings so we can swap between them
	UPROPERTY(EditDefaultsOnly, Category = "Config|Movement")
	float RunSpeed = 450.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config|Movement")
	float WalkSpeed = 180.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config|Movement")
	float SprintSpeed = 600.f;
};

