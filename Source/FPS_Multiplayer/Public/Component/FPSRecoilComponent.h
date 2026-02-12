// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSRecoilComponent.generated.h"

class UFPSWeaponData;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPS_MULTIPLAYER_API UFPSRecoilComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UFPSRecoilComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	/** Called by the Weapon when it fires. Adds force to the system. */
	UFUNCTION(BlueprintCallable, Category = "RecoilComponent")
	void ApplyRecoil(const FVector& RecoilForce, float InterpSpeed, float RecoverySpeed);

	/** Called by the Weapon when firing stops. Enables recovery logic. */
	UFUNCTION(BlueprintCallable, Category = "RecoilComponent")
	void StartRecovery();

protected:

	virtual void BeginPlay() override;

private:
	
	// =========================================================================
	//                       RECOIL INTERPOLATION STATE 
	// =========================================================================
    
	/** The final rotation we want to reach (The "Top" of the kick). */
	FRotator TargetRecoilRotation = FRotator::ZeroRotator;
    
	/** The rotation currently applied to the camera. */
	FRotator CurrentRecoilRotation = FRotator::ZeroRotator;
    
	/** The rotation we aim to return to (usually ZeroRotator). */
	FRotator RecoveryTargetRotation = FRotator::ZeroRotator;

	bool bIsRecovering = false;
	float CurrentInterpSpeed = 30.f;
	float CurrentRecoverySpeed = 20.f;
};
