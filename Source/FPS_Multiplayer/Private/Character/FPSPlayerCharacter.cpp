// © 2025 Heathrow (Derman Can Danisman). All rights reserved.


#include "FPS_Multiplayer/Public/Character/FPSPlayerCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Actor/Weapon/FPSWeapon.h"
#include "Animation/FPSAnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Component/FPSCombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "UniversalObjectLocators/AnimInstanceLocatorFragment.h"


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
		
		// Aim (RMB)
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &ThisClass::OnAimPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ThisClass::OnAimReleased);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Canceled, this, &ThisClass::OnAimReleased);
		
		// Interaction (ex: F Key)
		EnhancedInputComponent->BindAction(InteractionAction, ETriggerEvent::Started, this, &ThisClass::Interact);
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
	SetAimState(EAimState::EAS_ADS);
	if (IsLocallyControlled())
	{
		UFPSAnimInstance* AnimInstance = Cast<UFPSAnimInstance>(GetMesh()->GetAnimInstance());
		if (AnimInstance)
		{
			AnimInstance->CalculateHandTransforms();
		}
	}
}

void AFPSPlayerCharacter::OnAimReleased()
{
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


void AFPSPlayerCharacter::Interact(const FInputActionValue& Value)
{
	// Check if we have a valid weapon to grab
	AFPSWeapon* WeaponToEquip = CombatComponent->GetOverlappingWeapon();
	if (!WeaponToEquip) return;

	// NEW WAY (Universal Function):
	// Works for Server Host OR Client. The Component figures it out.
	CombatComponent->EquipWeapon(WeaponToEquip);
}

void AFPSPlayerCharacter::UpdateMovementSettings(const FWeaponMovementData& NewData)
{
	// 1. Update Physical Limits (The Capsule)
	GetCharacterMovement()->MaxWalkSpeed = NewData.MaxBaseSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = NewData.MaxCrouchSpeed;

	// 2. Update Sprint Speed (If you have a separate Sprint System)
	// SprintSpeed = NewData.MaxSprintSpeed;
}

void AFPSPlayerCharacter::SetOverlayState(EOverlayState NewState)
{
	LayerStates.OverlayState = NewState;
}