// © 2025 Heathrow (Derman Can Danisman). All rights reserved.


#include "FPS_Multiplayer/Public/Character/FPSPlayerCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Actor/Weapon/FPSWeapon.h"
#include "Animation/FPSAnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Component/FPSCombatComponent.h"
#include "Component/FPSInteractionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"


AFPSPlayerCharacter::AFPSPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	/*// Create the Shadow Mesh
	ShadowMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ShadowMesh"));
	ShadowMesh->SetupAttachment(GetMesh()); // Attach to main mesh logic
    
	// CRITICAL SETTINGS:
	ShadowMesh->SetCastShadow(true);            // We WANT shadows
	ShadowMesh->SetRenderInMainPass(false);     // We DO NOT want to see the mesh itself
	ShadowMesh->SetCastHiddenShadow(true);      // Allow casting even if "hidden" logic applies
    
	// Optimization: Disable collision and ticking (it will follow Master Pose)
	ShadowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShadowMesh->SetComponentTickEnabled(false);*/
	
	// 1. CAMERA SETUP (FPS Style)
	// We usually attach the camera directly to the mesh or a socket for FPS,
	// but a SpringArm is fine if you want flexibility (e.g., death cam).
	/*SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(GetMesh(), FName("CameraSocket")); // Ensure this socket exists on your mesh!
	SpringArmComponent->TargetArmLength = 0.0f; // FPS view usually has 0 length
	SpringArmComponent->bUsePawnControlRotation = true; // Rotate arm with controller*/

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(GetMesh(), FName("CameraSocket"));
	CameraComponent->bUsePawnControlRotation = true; // Camera follows the arm
	CameraComponent->bEnableFirstPersonFieldOfView = true;
	CameraComponent->bEnableFirstPersonScale = true;
	
	CombatComponent = CreateDefaultSubobject<UFPSCombatComponent>("CombatComponent");
	CombatComponent->SetIsReplicated(true);
	
	InteractionComponent = CreateDefaultSubobject<UFPSInteractionComponent>("InteractionComponent");

	// 2. MOVEMENT SETTINGS
	// For FPS, we typically want the character to rotate with the camera
	bUseControllerRotationYaw = true;
	// Disable standard camera smoothing because our SpringArm/Camera is attached to the Mesh Socket.
	// We want the Animation to drive the camera movement 100%.
	GetCharacterMovement()->bCrouchMaintainsBaseLocation = true;
	GetCharacterMovement()->bOrientRotationToMovement = false; // Strafing behavior
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanWalk = true;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanJump = true;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanSwim = true;
}

void AFPSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsLocallyControlled())
	{
		APlayerController* PC = GetController<APlayerController>();
		if (PC)
		{
			UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
			if (Subsystem)
			{
				if (PlayerCharacterMappingContext)
				{
					Subsystem->AddMappingContext(PlayerCharacterMappingContext, 0);
				}
			}
		}
	}
	
	/*if (IsLocallyControlled())
	{
		// 1. Setup Main Mesh (Visuals) - HIDE HEAD
		GetMesh()->HideBoneByName(FName("head"), EPhysBodyOp::PBO_None);

		// 2. Setup Shadow Mesh (Shadows) - SHOW HEAD
        
		// A. Ensure it uses the same mesh asset
		ShadowMesh->SetSkinnedAssetAndUpdate(GetMesh()->GetSkinnedAsset());

		// C. Visibility Settings
		ShadowMesh->SetCastShadow(true);
		ShadowMesh->SetRenderInMainPass(false); // Invisible to camera
		ShadowMesh->SetCastHiddenShadow(true);  // Casts shadow anyway
		ShadowMesh->SetHiddenInGame(false);     // Component is "Active"
	}
	else
	{
		// Remote players just see the main mesh normally
		ShadowMesh->SetHiddenInGame(true);
		ShadowMesh->SetCastShadow(false); 
	}*/
}

void AFPSPlayerCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION(ThisClass, LayerStates, COND_None);
}

void AFPSPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AFPSPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	// 4. BIND ACTIONS
	// We cast to EnhancedInputComponent to bind our UInputActions
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Move (WASD)
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);

		// Look (Mouse)
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);

		// Jump (Spacebar)
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ThisClass::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::StopJumping);
		
		// Crouch (Left-CTRL)
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &ThisClass::OnCrouchPressed);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ThisClass::OnCrouchReleased);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Canceled, this, &ThisClass::OnCrouchReleased);
		
		// Walk (Left-Alt)
		EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Triggered, this, &ThisClass::OnWalkPressed);
		EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Completed, this, &ThisClass::OnWalkReleased);
		EnhancedInputComponent->BindAction(WalkAction, ETriggerEvent::Canceled, this, &ThisClass::OnWalkReleased);
		
		// Sprint (Left-Shift)
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &ThisClass::OnSprintPressed);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ThisClass::OnSprintReleased);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ThisClass::OnSprintReleased);
		
		// Fire (LMB)
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ThisClass::OnFirePressed);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Ongoing, this, &ThisClass::OnFirePressed);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ThisClass::OnFireReleased);
		
		// Reload (R)
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ThisClass::OnReloadPressed);
		
		// Aim (RMB)
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &ThisClass::OnAimPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ThisClass::OnAimReleased);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Canceled, this, &ThisClass::OnAimReleased);
		
		// Interaction (ex: F Key)
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ThisClass::OnInteractedPressed);
	}
}

void AFPSPlayerCharacter::Move(const FInputActionValue& Value)
{
	// Read the Vector2D (X = Left/Right, Y = Forward/Back)
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Find out which way is "Forward" based on the camera's rotation
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// Get Forward/Right Vectors
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Apply Movement
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AFPSPlayerCharacter::Look(const FInputActionValue& Value)
{
	// Read the Vector2D (X = Mouse X, Y = Mouse Y)
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Apply to Controller (which rotates the view)
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AFPSPlayerCharacter::OnWalkPressed()
{
	// 1. Update Speed Locally (Prediction)
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// 2. Update State
	SetGaitState(EGait::EG_Walking);
}

void AFPSPlayerCharacter::OnWalkReleased()
{
	// 1. Restore Speed Locally
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

	// 2. Update State
	SetGaitState(EGait::EG_Running);
}

void AFPSPlayerCharacter::OnSprintPressed()
{
	bWantsToSprint = true; 
	TryStartSprinting();
}
void AFPSPlayerCharacter::OnSprintReleased()
{
	bWantsToSprint = false; 
    
	// If we are currently sprinting, stop.
	if (LayerStates.Gait == EGait::EG_Sprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		SetGaitState(EGait::EG_Running);
	}
}

void AFPSPlayerCharacter::OnCrouchPressed()
{
	if (CanCrouch())
	{
		Crouch();
	}
}

void AFPSPlayerCharacter::OnCrouchReleased()
{
	UnCrouch();
}

void AFPSPlayerCharacter::OnStartCrouch(float HeightAdjust, float ScaledHeightAdjust)
{
	Super::OnStartCrouch(HeightAdjust, ScaledHeightAdjust);
	
	// This runs on:
	// 1. Server (Authority)
	// 2. Client (immediately via prediction)
	// 3. Simulated Proxies (when bIsCrouched replicates)
	LayerStates.Stance = EStance::ES_Crouching;
}

void AFPSPlayerCharacter::OnEndCrouch(float HeightAdjust, float ScaledHeightAdjust)
{
	Super::OnEndCrouch(HeightAdjust, ScaledHeightAdjust);
	
	LayerStates.Stance = EStance::ES_Standing;
}

void AFPSPlayerCharacter::OnFirePressed()
{
	// 1. INTERRUPT: Sprint-Out
	// If we are sprinting, force the state to Running immediately.
	StopSprinting(); // 1. Handle Interrupts

	// 2. PERMISSION
	// Now that Gait is 'Running', CanFire() will pass (assuming we have ammo).
	if (CanFire())
	{
		CombatComponent->StartFire();
	}
}

void AFPSPlayerCharacter::OnFireReleased()
{
	CombatComponent->StopFire();
	if (bWantsToSprint) TryStartSprinting(); // Resume!
}

void AFPSPlayerCharacter::OnReloadPressed()
{
	// 1. INTERRUPT: Sprint-Out
	// If we are sprinting, force the state to Running immediately.
	StopSprinting(); // 1. Handle Interrupts
    
	// 2. PERMISSION
	if (CanReload())
	{
		CombatComponent->Reload();
	}
}

void AFPSPlayerCharacter::OnAimPressed()
{
	// 1. INTERRUPT: Sprint-Out
	// If we are sprinting, force the state to Running immediately.
	StopSprinting(); // 1. Handle Interrupts
	
	if (CanAim())
	{
		SetAimState(EAimState::EAS_ADS);
	}
}

void AFPSPlayerCharacter::OnAimReleased()
{
	SetAimState(EAimState::EAS_None);
	if (bWantsToSprint) TryStartSprinting(); // Resume!
}

void AFPSPlayerCharacter::SetAimState(EAimState NewState)
{
	// 1. Prediction: Update locally immediately so it feels responsive
	LayerStates.AimState = NewState;
	
	// 2. Replication: Tell the Server
	if (!HasAuthority())
	{
		Server_SetAimState(NewState);
	}
}

void AFPSPlayerCharacter::Server_SetAimState_Implementation(EAimState NewState)
{
	// Server updates its authoritative copy
	// This will replicate down to all clients via OnRep_CharacterLayers
	LayerStates.AimState = NewState;
}


void AFPSPlayerCharacter::OnInteractedPressed(const FInputActionValue& Value)
{
	if (InteractionComponent)
	{
		InteractionComponent->PrimaryInteract();
	}
}

void AFPSPlayerCharacter::SetGaitState(EGait NewState)
{
	// 1. Prediction: Update locally immediately
	LayerStates.Gait = NewState;

	// 2. Replication: Tell the Server
	if (!HasAuthority())
	{
		Server_SetGaitState(NewState);
	}
}

void AFPSPlayerCharacter::Server_SetGaitState_Implementation(EGait NewState)
{
	// Server updates authoritative copy
	LayerStates.Gait = NewState;
    
	// CRITICAL: This MUST match Client's UpdateMovementSettings logic exactly!
	if (NewState == EGait::EG_Sprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
	else if (NewState == EGait::EG_Walking)
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
	else // Running
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void AFPSPlayerCharacter::UpdateMovementSettings(const FWeaponMovementData& NewData)
{
	// 1. Cache the values so we can swap back and forth
	BaseWalkSpeed = NewData.MaxBaseSpeed;
	WalkSpeed = NewData.AnimWalkRefSpeed;
	SprintSpeed = NewData.AnimSprintRefSpeed; // Assumes this exists in your struct
	GetCharacterMovement()->MaxWalkSpeedCrouched = NewData.MaxCrouchSpeed;

	// 2. Apply the correct speed based on current state
	if (GetGaitState() == EGait::EG_Walking)
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
	else if (GetGaitState() == EGait::EG_Sprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void AFPSPlayerCharacter::SetOverlayState(EOverlayState NewState)
{
	LayerStates.OverlayState = NewState;
}

// =========================================================================
//                        STATE GATEKEEPERS (Rulesets)
// =========================================================================

bool AFPSPlayerCharacter::CanSprint() const
{
	if (GetCombatComponent()->GetCombatState() != ECombatState::ECS_Unoccupied) return false;
	if (GetCharacterMovement()->IsFalling()) return false;
	return true;
}

bool AFPSPlayerCharacter::CanCrouch() const
{
	if (GetCharacterMovement()->IsFalling()) return false;
	return Super::CanCrouch();
}

bool AFPSPlayerCharacter::CanAim() const
{
	// Safety First: If the component is missing, we definitely can't aim.
	if (!GetCombatComponent()) return false;
	
	// STRICT RULE: Cannot aim if sprinting.
	if (LayerStates.Gait == EGait::EG_Sprinting) return false;
	
	if (GetCombatComponent()->GetEquippedWeapon() == nullptr) return false;
	if (GetCombatComponent()->GetCombatState() != ECombatState::ECS_Unoccupied) return false;
	return true;
}

bool AFPSPlayerCharacter::CanFire() const
{
	// Safety First: If the component is missing, we definitely can't fire.
	if (!CombatComponent || !CombatComponent->GetEquippedWeapon()) return false;
	
	// STRICT RULE: Cannot fire if sprinting.
	// Use the LayerState (Replicated), not the Component state, to ensure client/server agree.
	if (LayerStates.Gait == EGait::EG_Sprinting) return false;
	
	if (GetCombatComponent()->GetEquippedWeapon() == nullptr) return false;
	if (GetCombatComponent()->GetCombatState() != ECombatState::ECS_Unoccupied) return false;
	if (GetCombatComponent()->GetEquippedWeapon()->GetCurrentClipAmmo() <= 0) return false;
	return true;
}

bool AFPSPlayerCharacter::CanReload() const
{
	// Safety First: If the component is missing, we definitely can't reload.
	if (!GetCombatComponent()) return false;
	
	// STRICT RULE: Cannot reload if sprinting.
	if (LayerStates.Gait == EGait::EG_Sprinting) return false;
	
	if (GetCombatComponent()->GetEquippedWeapon() == nullptr) return false;
	if (GetCombatComponent()->GetCombatState() != ECombatState::ECS_Unoccupied) return false;
	if (GetCombatComponent()->GetEquippedWeapon()->GetCurrentClipAmmo() >= GetCombatComponent()->GetEquippedWeapon()->GetMaxClipAmmo()) return false;
	if (GetCombatComponent()->GetCarriedAmmo() <= 0) return false;
	if (GetCharacterMovement()->IsFalling()) return false;
	return true;
}

void AFPSPlayerCharacter::StopSprinting()
{
	if (LayerStates.Gait == EGait::EG_Sprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		SetGaitState(EGait::EG_Running);
	}
}

// =========================================================================
//                       HELPER FUNCTIONS
// =========================================================================

void AFPSPlayerCharacter::TryStartSprinting()
{
	// Local Check: Do we want to sprint? Are we allowed to?
	if (bWantsToSprint && CanSprint()) 
	{
		// EXECUTE (Prediction)
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
		SetGaitState(EGait::EG_Sprinting); // This calls Server RPC internally!
	}
}
