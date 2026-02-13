// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "Data/Enums/FPSCharacterTypes.h"
#include "Data/Structs/FPSCharacterDataContainer.h"
#include "GameFramework/Character.h"
#include "Interface/FPSWeaponHandlerInterface.h"
#include "FPSPlayerCharacter.generated.h"

class UFPSInteractionComponent;
struct FWeaponMovementData;
class UFPSCombatComponent;
class AFPSWeapon;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UCameraComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGaitChanged, EGait);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnOverlayStateChanged, EOverlayState);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAimStateChanged, EAimState);

UCLASS(PrioritizeCategories=("Components Player"))
class FPS_MULTIPLAYER_API AFPSPlayerCharacter : public ACharacter, public IFPSWeaponHandlerInterface
{
	GENERATED_BODY()

public:
	AFPSPlayerCharacter();
	
	virtual void PossessedBy(AController* NewController) override;
	
	virtual void OnRep_PlayerState() override;
	
	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// Called by CombatComponent when a weapon is equipped
	void UpdateMovementSettings(const FWeaponMovementData& NewData);
	
	// =========================================================================
	//                        DELEGATES
	// =========================================================================
	    
	FOnGaitChanged OnGaitStateChanged;
	FOnOverlayStateChanged OnOverlayStateChanged;
	FOnAimStateChanged OnAimStateChanged;
	
	// =========================================================================
	//                        GETTER FUNCTIONS
	// =========================================================================
	
	UFUNCTION(BlueprintCallable, Category = "Player|Getters")
	FORCEINLINE UCameraComponent* GetCameraComponent() const { return CameraComponent; }
	
	UFUNCTION(BlueprintCallable, Category = "Player|Getters")
	FORCEINLINE UFPSCombatComponent* GetCombatComponent() const { return CombatComponent; }
	
	UFUNCTION(BlueprintCallable, Category = "Player|Getters|Character States")
	FORCEINLINE FCharacterLayerStates GetLayerStates() const { return LayerStates; }
	
	UFUNCTION(BlueprintCallable, Category = "Player|Getters|Character States")
	FORCEINLINE EGait GetGaitState() const { return LayerStates.Gait; }
	
	UFUNCTION(BlueprintCallable, Category = "Player|Getters|Character States")
	FORCEINLINE EOverlayState GetOverlayState() const { return LayerStates.OverlayState; }
	
	UFUNCTION(BlueprintCallable, Category = "Player|Getters|Character States")
	FORCEINLINE EAimState GetAimState() const { return LayerStates.AimState; }
	
	// =========================================================================
	//                        SETTER FUNCTIONS
	// =========================================================================
	
	// Helper to change Gait state (Handles Local Prediction + RPC)
	UFUNCTION(BlueprintCallable, Category = "Player|Character States")
	void SetGaitState(EGait NewState);
	
	UFUNCTION(BlueprintCallable, Category = "Player|Character States")
	void SetOverlayState(EOverlayState NewState);
	
	// Helper to change state (Handles Local Prediction + RPC)
	UFUNCTION(BlueprintCallable, Category = "Player|Character States")
	void SetAimState(EAimState NewState);
	
	// =========================================================================
	//               IWeaponHandlerInterface Implementation
	// =========================================================================
	
	virtual void SetCurrentWeapon(AFPSWeapon* WeaponToEquip) override;
	virtual void PlayFireMontage(UAnimMontage* HipFireMontage, UAnimMontage* AimedFireMontage) override;

protected:

	virtual void BeginPlay() override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void OnStartCrouch(float HeightAdjust, float ScaledHeightAdjust) override;
	virtual void OnEndCrouch(float HeightAdjust, float ScaledHeightAdjust) override;
	
	// =========================================================================
	//                        ENHANCED INPUT CONFIGURATION
	// =========================================================================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Enhanced Input")
	TObjectPtr<UInputMappingContext> PlayerCharacterMappingContext;
	
	// The specific actions
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Enhanced Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Enhanced Input")
	TObjectPtr<UInputAction> LookAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Enhanced Input")
	TObjectPtr<UInputAction> WalkAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Enhanced Input")
	TObjectPtr<UInputAction> JumpAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Enhanced Input")
	TObjectPtr<UInputAction> CrouchAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Enhanced Input")
	TObjectPtr<UInputAction> SprintAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Enhanced Input")
	TObjectPtr<UInputAction> FireAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Enhanced Input")
	TObjectPtr<UInputAction> ReloadAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Enhanced Input")
	TObjectPtr<UInputAction> AimAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Enhanced Input")
	TObjectPtr<UInputAction> InteractAction;
	
	// A duplicate mesh used ONLY to cast shadows for the local player.
	// It follows the Main Mesh animations perfectly using Master Pose.
	/*UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> ShadowMesh;*/
	
	// =========================================================================
	//                        INPUT FUNCTIONS
	// =========================================================================
	
	// Called when WASD is pressed
	void Move(const FInputActionValue& Value);

	// Called when Mouse moves
	void Look(const FInputActionValue& Value);
	
	void OnWalkPressed();
	void OnWalkReleased();

	void OnSprintPressed();
	void OnSprintReleased();
	
	void OnCrouchPressed();
	void OnCrouchReleased(); // Optional: If you want "Hold to Crouch"
	
	void OnFirePressed();
	void OnFireReleased();
	
	void OnReloadPressed();
	
	// Input Handlers
	void OnAimPressed();
	void OnAimReleased();
	
	// Called when Interaction Key is pressed
	void OnInteractedPressed(const FInputActionValue& Value);
	
	// =========================================================================
	//                        STATE GATEKEEPERS (Rulesets)
	// =========================================================================
    
	/** Returns true if the player is legally allowed to sprint. */
	bool CanSprint() const;
	
	/** Returns true if the player is legally allowed to crouch. */
	virtual bool CanCrouch() const override;
    
	/** Returns true if the player is legally allowed to aim. */
	bool CanAim() const;
    
	/** Returns true if the weapon and player states allow firing. */
	bool CanFire() const;
    
	/** Returns true if the weapon and player states allow reloading. */
	bool CanReload() const;
	
	/**
	 * Forcefully stops sprinting and resets movement speed.
	 * Called when performing actions that block sprinting (Firing, Reloading).
	 */
	void StopSprinting();

	// =========================================================================
	//                       SERVER RPCs
	// =========================================================================
	
	UFUNCTION(Server, Reliable)
	void Server_SetGaitState(EGait NewState);
	
	UFUNCTION(Server, Reliable)
	void Server_SetAimState(EAimState NewState);
	
	UPROPERTY(Replicated)
	FCharacterLayerStates LayerStates;

private:
	
	/*UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArmComponent;*/
	
	UPROPERTY(VisibleAnywhere, Category = "Components|Camera")
	TObjectPtr<UCameraComponent> CameraComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Components|Actor Components")
	TObjectPtr<UFPSCombatComponent> CombatComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Components|Actor Components")
	TObjectPtr<UFPSInteractionComponent> InteractionComponent;
	
	// Cache these values from UpdateMovementSettings so we can swap between them
	UPROPERTY(EditDefaultsOnly, Category = "Player|Config|Movement")
	float BaseWalkSpeed = 300.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Player|Config|Movement")
	float WalkSpeed = 150.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Player|Config|Movement")
	float SprintSpeed = 600.f;
	
	// =========================================================================
	//                       HELPER FUNCTIONS & VARIABLES
	// =========================================================================
	
	bool bWantsToSprint = false;
	
	void TryStartSprinting();
};