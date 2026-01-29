// © 2025 Heathrow (Derman Can Danisman). All rights reserved.


#include "FPS_Multiplayer/Public/Character/FPSPlayerCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"


AFPSPlayerCharacter::AFPSPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Create the Shadow Mesh
	ShadowMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ShadowMesh"));
	ShadowMesh->SetupAttachment(GetMesh()); // Attach to main mesh logic
    
	// CRITICAL SETTINGS:
	ShadowMesh->SetCastShadow(true);            // We WANT shadows
	ShadowMesh->SetRenderInMainPass(false);     // We DO NOT want to see the mesh itself
	ShadowMesh->SetCastHiddenShadow(true);      // Allow casting even if "hidden" logic applies
    
	// Optimization: Disable collision and ticking (it will follow Master Pose)
	ShadowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShadowMesh->SetComponentTickEnabled(false);
	
	// 1. CAMERA SETUP (FPS Style)
	// We usually attach the camera directly to the mesh or a socket for FPS,
	// but a SpringArm is fine if you want flexibility (e.g., death cam).
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
	SpringArmComponent->SetupAttachment(GetMesh(), FName("CameraSocket")); // Ensure this socket exists on your mesh!
	SpringArmComponent->TargetArmLength = 0.0f; // FPS view usually has 0 length
	SpringArmComponent->bUsePawnControlRotation = true; // Rotate arm with controller

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(SpringArmComponent);
	CameraComponent->bUsePawnControlRotation = false; // Camera follows the arm

	// 2. MOVEMENT SETTINGS
	// For FPS, we typically want the character to rotate with the camera
	bUseControllerRotationYaw = true; 
	GetCharacterMovement()->bOrientRotationToMovement = false; // Strafing behavior
}

void AFPSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// 3. REGISTER INPUT MAPPING
	// This tells the PlayerController: "I am using the 'DefaultMappingContext' (WASD/Mouse)"
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		if (Subsystem)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	if (IsLocallyControlled())
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
	}
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
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
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

