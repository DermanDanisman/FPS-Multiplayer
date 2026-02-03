// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "Enums/CharacterTypes.h"
#include "GameFramework/Character.h"
#include "Structs/CharacterDataContainer.h"
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
	
	UFUNCTION(BlueprintCallable, Category = "Camera")
	FORCEINLINE UCameraComponent* GetCameraComponent() const { return CameraComponent; }
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	FORCEINLINE UFPSCombatComponent* GetCombatComponent() const { return CombatComponent; }
	
	// Called by CombatComponent when a weapon is equipped
	void UpdateMovementSettings(const FWeaponMovementData& NewData);
	
	UFUNCTION(BlueprintCallable, Category = "Character States")
	void SetOverlayState(EOverlayState NewState);
	
	UFUNCTION(BlueprintCallable, Category = "Character States")
	FORCEINLINE FCharacterLayerStates GetLayerStates() const { return LayerStates; }
	
	// Helper to change state (Handles Local Prediction + RPC)
	UFUNCTION(BlueprintCallable, Category = "Character States")
	void SetAimState(EAimState NewState);

protected:

	virtual void BeginPlay() override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void OnStartCrouch(float HeightAdjust, float ScaledHeightAdjust) override;
	virtual void OnEndCrouch(float HeightAdjust, float ScaledHeightAdjust) override;
	
	// --- ENHANCED INPUT CONFIGURATION ---

	// The specific actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> JumpAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> CrouchAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> AimAction;
	
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
	
	void OnCrouchPressed();
	void OnCrouchReleased(); // Optional: If you want "Hold to Crouch"
	
	// Input Handlers
	void OnAimPressed();
	void OnAimReleased();
	
	// --- RPCs ---
	UFUNCTION(Server, Reliable)
	void Server_SetAimState(EAimState NewState);
	
	// Called when Interaction Key is pressed
	void Interact(const FInputActionValue& Value);
	
protected:
	
	UPROPERTY(Replicated)
	FCharacterLayerStates LayerStates;

private:
	
	/*UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArmComponent;*/
	
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UFPSCombatComponent> CombatComponent;
};