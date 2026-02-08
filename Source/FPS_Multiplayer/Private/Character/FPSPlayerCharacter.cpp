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
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanWalk = true;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanJump = true;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanSwim = true;
}

void AFPSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
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
		
		// Sprint (Left-Shift)
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &ThisClass::OnSprintPressed);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ThisClass::OnSprintReleased);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ThisClass::OnSprintReleased);
		
		// Aim (RMB)
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &ThisClass::OnAimPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ThisClass::OnAimReleased);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Canceled, this, &ThisClass::OnAimReleased);
		
		// Interaction (ex: F Key)
		EnhancedInputComponent->BindAction(InteractionAction, ETriggerEvent::Started, this, &ThisClass::OnInteractedPressed);
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

void AFPSPlayerCharacter::OnCrouchPressed()
{
	if (!GetCharacterMovement()->IsCrouching())
	{
		if (!GetCharacterMovement()->IsFalling())
		{
			Crouch();
		}
	}
}

void AFPSPlayerCharacter::OnCrouchReleased()
{
	UnCrouch();
}

void AFPSPlayerCharacter::OnSprintPressed()
{
	// Block sprint if crouching (optional design choice)
	if (LayerStates.Stance == EStance::ES_Crouching) 
	{
		UnCrouch(); // Or return; if you want crouch to block sprint
	}

	// Sprint-Out: If we are aiming, stop aiming to allow sprint
	if (LayerStates.AimState == EAimState::EAS_ADS)
	{
		SetAimState(EAimState::EAS_None);
	}

	// 1. Update Speed Locally (Prediction)
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

	// 2. Update State
	SetGaitState(EGait::EG_Sprinting);
}

void AFPSPlayerCharacter::OnSprintReleased()
{
	// 1. Restore Speed Locally
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

	// 2. Update State
	SetGaitState(EGait::EG_Running); // Or EG_Walking, depending on your default
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

void AFPSPlayerCharacter::OnAimPressed()
{
	if (!IsValid(CombatComponent->GetEquippedWeapon())) return;
	
	SetAimState(EAimState::EAS_ADS);
}

void AFPSPlayerCharacter::OnAimReleased()
{
	if (!IsValid(CombatComponent->GetEquippedWeapon())) return;
	
	SetAimState(EAimState::EAS_None);
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
    
	// Server must also update the physical movement speed to prevent rubber-banding
	if (NewState == EGait::EG_Sprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void AFPSPlayerCharacter::UpdateMovementSettings(const FWeaponMovementData& NewData)
{
	// 1. Cache the values so we can swap back and forth
	BaseWalkSpeed = NewData.MaxBaseSpeed;
	SprintSpeed = NewData.AnimSprintRefSpeed; // Assumes this exists in your struct
	GetCharacterMovement()->MaxWalkSpeedCrouched = NewData.MaxCrouchSpeed;

	// 2. Apply the correct speed based on current state
	if (LayerStates.Gait == EGait::EG_Sprinting)
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