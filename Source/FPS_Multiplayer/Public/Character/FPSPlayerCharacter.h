// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/Character.h"
#include "FPSPlayerCharacter.generated.h"

struct FWeaponMovementData;
class UFPSCombatComponent;
class AFPSWeapon;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class FPS_MULTIPLAYER_API AFPSPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFPSPlayerCharacter();
	
	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	FORCEINLINE UFPSCombatComponent* GetCombatComponent() const { return CombatComponent; }
	
	// Called by CombatComponent when a weapon is equipped
	void UpdateMovementSettings(const FWeaponMovementData& NewData);

protected:

	virtual void BeginPlay() override;
	
	// --- ENHANCED INPUT CONFIGURATION ---
	
	// The "Map" of keys. (e.g. WASD -> Move, Mouse -> Look)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	// The specific actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> JumpAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> InteractionAction;
	
	// A duplicate mesh used ONLY to cast shadows for the local player.
	// It follows the Main Mesh animations perfectly using Master Pose.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> ShadowMesh;

	// --- INPUT FUNCTIONS ---
	
	// Called when WASD is pressed
	void Move(const FInputActionValue& Value);

	// Called when Mouse moves
	void Look(const FInputActionValue& Value);
	
	// Called when Interaction Key is pressed
	void Interact(const FInputActionValue& Value);

private:
	
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArmComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UFPSCombatComponent> CombatComponent;
};
