// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "Data/Enums/FPSCharacterTypes.h"
#include "Data/Structs/FPSCharacterDataContainer.h"
#include "GameFramework/Character.h"
#include "Interface/FPSWeaponHandlerInterface.h"
#include "Serialization/Archive.h" // Needed for serialization
#include "FPSPlayerCharacter.generated.h"

class UWidgetComponent;
class UTurnInPlace;
class UFPSCharacterMovementComponent;
class UFPSInteractionComponent;
class UFPSCombatComponent;
class AFPSWeapon;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UCameraComponent;

/**
 * @struct FReplicatedControlRotation
 * @brief  Optimized network structure for replicating Control Rotation (Aim Direction).
 * * NETWORK OPTIMIZATION EXPLAINED:
 * 1. Standard 'FRotator' uses 3x 32-bit floats (96 bits total).
 * 2. For Aim Offsets, we only need Pitch and Yaw (Roll is irrelevant).
 * 3. We compress 360 degrees into a 16-bit integer (0 to 65535).
 * * RESULT: 
 * - Bandwidth usage drops from 96 bits -> 32 bits (66% reduction).
 * - Precision is ~0.005 degrees per step (Imperceptible to human eye).
 * - Replaces the legacy "RemoteViewPitch" byte which was too jittery (1.4 deg precision).
 */
USTRUCT()
struct FReplicatedControlRotation
{
	GENERATED_BODY()

	// Pitch: Up/Down look angle. 0-65535 maps to 0-360 degrees.
	UPROPERTY()
	uint16 Pitch = 0;

	// Yaw: Left/Right look angle. 0-65535 maps to 0-360 degrees.
	UPROPERTY()
	uint16 Yaw = 0;

	// --- HELPER: COMPRESS (Float -> Int) ---
	void SetFromRotator(const FRotator& InRotator)
	{
		// 1. Normalize axes to ensure we are strictly in 0-360 range (handles negatives like -90)
		float NormalizedPitch = FRotator::NormalizeAxis(InRotator.Pitch);
		float NormalizedYaw = FRotator::NormalizeAxis(InRotator.Yaw);

		// 2. Map Range: 
		// We multiply by 65535 (max uint16) and divide by 360 (max degrees).
		// Formula: (Angle / 360.0) * 65535.0
		Pitch = FMath::RoundToInt((NormalizedPitch / 360.f) * 65535.f);
		Yaw = FMath::RoundToInt((NormalizedYaw / 360.f) * 65535.f);
	}

	// --- HELPER: DECOMPRESS (Int -> Float) ---
	FRotator ToRotator() const
	{
		// 1. Reverse the Math:
		// Formula: (Step / 65535.0) * 360.0
		float DecompressedPitch = (Pitch / 65535.f) * 360.f;
		float DecompressedYaw = (Yaw / 65535.f) * 360.f;

		// 2. Return standard rotator (Roll is always 0 for aim offsets)
		return FRotator(DecompressedPitch, DecompressedYaw, 0.f);
	}
};

/**
 * FLyraReplicatedAcceleration: Compressed representation of acceleration
 */
USTRUCT()
struct FReplicatedAcceleration
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 AccelXYRadians = 0;	// Direction of XY accel component, quantized to represent [0, 2*pi]

	UPROPERTY()
	uint8 AccelXYMagnitude = 0;	//Accel rate of XY component, quantized to represent [0, MaxAcceleration]

	UPROPERTY()
	int8 AccelZ = 0;	// Raw Z accel rate component, quantized to represent [-MaxAcceleration, MaxAcceleration]
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGaitChanged, EGait);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnOverlayStateChanged, EOverlayState);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAimStateChanged, EAimState);

UCLASS()
class FPS_MULTIPLAYER_API AFPSPlayerCharacter : public ACharacter, public IFPSWeaponHandlerInterface
{
	GENERATED_BODY()

public:
	AFPSPlayerCharacter(const FObjectInitializer& ObjectInitializer);
    
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	// =========================================================================
	//                        DELEGATES
	// =========================================================================
	    
	FOnGaitChanged OnGaitStateChanged;
	FOnOverlayStateChanged OnOverlayStateChanged;
	FOnAimStateChanged OnAimStateChanged;
	
	// =========================================================================
	//                        GETTER FUNCTIONS
	// =========================================================================
	
	UFUNCTION(BlueprintCallable, Category = "Getters")
	FORCEINLINE UCameraComponent* GetCameraComponent() const { return CameraComponent; }
	
	UFUNCTION(BlueprintCallable, Category = "Getters")
	FORCEINLINE UFPSCharacterMovementComponent* GetFPSMovementComponent() const { return FPSMovementComponent ? FPSMovementComponent : nullptr; }
	
	UFUNCTION(BlueprintCallable, Category = "Getters")
	FORCEINLINE UFPSCombatComponent* GetCombatComponent() const { return CombatComponent; }
	
	/** Getter so your Combat Component can revert to this when dropping weapons */
	TSubclassOf<UAnimInstance> GetUnarmedAnimLayerClass() const { return DefaultAnimLayerClass; }
	
	UFUNCTION(BlueprintCallable, Category = "Getters|Character States")
	FORCEINLINE FCharacterLayerStates GetLayerStates() const { return LayerStates; }
	
	UFUNCTION(BlueprintCallable, Category = "Getters|Character States")
	FORCEINLINE EGait GetGaitState() const { return LayerStates.Gait; }
	
	/**
	 * @brief Gets the Control Rotation efficiently based on Network Role.
	 * @return The smoothest available aim rotation.
	 * - Locally Controlled: Returns direct Input (Infinite precision, 0 latency).
	 * - Simulated Proxy: Returns decompressed Network Data (High precision, interpolated).
	 */
	UFUNCTION(BlueprintCallable, Category = "Getters")
	FRotator GetReplicatedControlRotation() const;

	// =========================================================================
	//                        SETTER FUNCTIONS
	// =========================================================================
	
	// Helper to change Gait state (Handles Local Prediction + RPC)
	UFUNCTION(BlueprintCallable, Category = "Character States")
	void SetGaitState(EGait NewState);
	
	UFUNCTION(BlueprintCallable, Category = "Character States")
	void SetOverlayState(EOverlayState NewState);
	
	// Helper to change state (Handles Local Prediction + RPC)
	UFUNCTION(BlueprintCallable, Category = "Character States")
	void SetAimState(EAimState NewState);
	
	// =========================================================================
	//               IWeaponHandlerInterface Implementation
	// =========================================================================
	
	virtual void SetCurrentWeapon(AFPSWeapon* WeaponToEquip) override;
	virtual void PlayFireMontage(UAnimMontage* HipFireMontage, UAnimMontage* AimedFireMontage) override;
	virtual void PlayEquipMontage(UAnimMontage* EquipMontage) override;
	virtual void PlayUnEquipMontage(UAnimMontage* UnEquipMontage) override;

	// =========================================================================
	//               BLUEPRINT IMPLEMENTABLE EVENTS
	// =========================================================================
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Character Events")
	void OnOverheadWidgetInitialized();
	
protected:

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual void OnStartCrouch(float HeightAdjust, float ScaledHeightAdjust) override;
	virtual void OnEndCrouch(float HeightAdjust, float ScaledHeightAdjust) override;
	
	// =========================================================================
	//                        ENHANCED INPUT CONFIGURATION
	// =========================================================================

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputMappingContext> PlayerCharacterMappingContext;
	
	// The specific actions
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> LookAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> WalkAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> JumpAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> CrouchAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> SprintAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> FireAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> ReloadAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> AimAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> InteractAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> PrimaryWeaponHotkeyAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> SecondaryWeaponHotkeyAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> SideArmWeaponHotkeyAction;
	
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
	
	void OnPrimaryWeaponHotkeyPressed(const FInputActionValue& Value);
	
	void OnSecondaryWeaponHotkeyPressed(const FInputActionValue& Value);
	
	void OnSideArmWeaponHotkeyPressed(const FInputActionValue& Value);
	
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
	
	UFUNCTION()
	void OnRep_LayerStates();

	/**
	 * Stores the compressed Aim Direction for other clients to read.
	 * @replicated Replicated to everyone EXCEPT the owner (COND_SkipOwner).
	 * Why SkipOwner? The owner already predicts their own input locally!
	 * A single 32-bit integer. (16 bits Pitch, 16 bits Yaw).
	 * This is the absolute cheapest way to send 2 rotation axes over a network.
	 */
	UPROPERTY(Replicated)
	uint32 ReplicatedControlRotation = 0;
	
	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedAcceleration)
	FReplicatedAcceleration ReplicatedAcceleration;
	
	UFUNCTION()
	void OnRep_ReplicatedAcceleration();
	
	// =========================================================================
	//                       ANIMATION LAYERS
	// =========================================================================
	
	/** * The default animation layer class used when the player spawns or drops all weapons.
	 * (e.g., ABP_Unarmed_Layers) 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TSubclassOf<UAnimInstance> DefaultAnimLayerClass;
	
	// =========================================================================
	//                       BLUEPRINT IMPLEMENTABLE EVENTS
	// =========================================================================
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon Events")
	void TriggerTimelineAimFOV(bool bShouldReverse);

private:
	
	UPROPERTY(VisibleAnywhere, Category = "Components|Camera")
	TObjectPtr<UCameraComponent> CameraComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Components|Movement")
	TObjectPtr<UFPSCharacterMovementComponent> FPSMovementComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Components|Actor Components")
	TObjectPtr<UFPSCombatComponent> CombatComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Components|Actor Components")
	TObjectPtr<UFPSInteractionComponent> InteractionComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "Components|Actor Components")
	TObjectPtr<UWidgetComponent> OverheadWidgetComponent;
	
	/*UPROPERTY(VisibleAnywhere, Category = "Weapons")
	TObjectPtr<USkeletalMeshComponent> PistolMesh;
	
	UPROPERTY(VisibleAnywhere, Category = "Weapons")
	TObjectPtr<USkeletalMeshComponent> RifleMesh;
	
	UPROPERTY(VisibleAnywhere, Category = "Weapons")
	TObjectPtr<USkeletalMeshComponent> ShotgunMesh;*/
	
	// =========================================================================
	//                       HELPER FUNCTIONS & VARIABLES
	// =========================================================================
	
	bool bWantsToSprint = false;
	
	bool bWantsToWalk = false;
	
	float DeltaSeconds;
	
	void InitializeOverheadWidget();
	
	void TryStartSprinting();
	
	void UpdateGait();
};