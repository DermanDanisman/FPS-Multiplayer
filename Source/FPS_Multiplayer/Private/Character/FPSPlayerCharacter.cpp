// © 2025 Heathrow (Derman Can Danisman). All rights reserved.


#include "FPS_Multiplayer/Public/Character/FPSPlayerCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Actor/Weapon/FPSWeapon.h"
#include "Animation/FPSAnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Component/FPSCharacterMovementComponent.h"
#include "Component/FPSCombatComponent.h"
#include "Component/FPSInteractionComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interface/FPSAnimInterface.h"
#include "Net/UnrealNetwork.h"
#include "Player/FPSPlayerController.h"
#include "Player/FPSPlayerState.h"
#include "UI/HUD/FPSHUD.h"
#include "UI/Widget/FPSPlayerInfoWidget.h"

AFPSPlayerCharacter::AFPSPlayerCharacter(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UFPSCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);
	SetNetUpdateFrequency(66.f);
	SetMinNetUpdateFrequency(33.f);
	
	// Ensure the mesh animates AFTER the character has physically moved this frame
	GetMesh()->AddTickPrerequisiteComponent(GetCharacterMovement());
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// =========================================================================
	// THE CAMERA
	// =========================================================================
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	// Attach to the Local Mesh's head so it moves with breathing/locomotion
	CameraComponent->SetupAttachment(GetMesh(), FName("CameraSocket"));
	CameraComponent->bUsePawnControlRotation = true;
	CameraComponent->bEnableFirstPersonFieldOfView = true;
	CameraComponent->bEnableFirstPersonScale = true;
	
	CombatComponent = CreateDefaultSubobject<UFPSCombatComponent>(TEXT("CombatComponent"));
	CombatComponent->SetIsReplicated(true);
	
	InteractionComponent = CreateDefaultSubobject<UFPSInteractionComponent>(TEXT("InteractionComponent"));
	
	OverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidgetComponent"));
	OverheadWidgetComponent->SetupAttachment(GetMesh(), FName("OverheadWidgetSocket"));
	OverheadWidgetComponent->SetOwnerNoSee(true);
	OverheadWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	OverheadWidgetComponent->SetDrawAtDesiredSize(true);

	// MOVEMENT SETTINGS
	bUseControllerRotationYaw = true; 
    FPSMovementComponent = CastChecked<UFPSCharacterMovementComponent>(ThisClass::GetMovementComponent());
	FPSMovementComponent->SetIsReplicated(true);
	FPSMovementComponent->bCrouchMaintainsBaseLocation = true;
	FPSMovementComponent->GravityScale = 1.0f;
	FPSMovementComponent->MaxAcceleration = 3200.0f;
	FPSMovementComponent->BrakingFrictionFactor = 1.0f;
	FPSMovementComponent->BrakingFriction = 6.0f;
	FPSMovementComponent->GroundFriction = 8.0f;
	FPSMovementComponent->BrakingDecelerationWalking = 3200.0f;
	FPSMovementComponent->bUseControllerDesiredRotation = false;
	FPSMovementComponent->bOrientRotationToMovement = false;
	FPSMovementComponent->MaxWalkSpeed = FPSMovementComponent->GetRunSpeed();
	FPSMovementComponent->GetNavAgentPropertiesRef().bCanWalk = true;
	FPSMovementComponent->GetNavAgentPropertiesRef().bCanCrouch = true;
	FPSMovementComponent->GetNavAgentPropertiesRef().bCanJump = true;
	FPSMovementComponent->GetNavAgentPropertiesRef().bCanSwim = true;
}

void AFPSPlayerCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION(ThisClass, LayerStates, COND_SkipOwner);
	
	// REPLICATION RULE: COND_SkipOwner
	// We do NOT send this packet to the player who owns this character.
	// Sending input back to the player who created it wastes bandwidth.
	DOREPLIFETIME_CONDITION(ThisClass, ReplicatedControlRotation, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(ThisClass, ReplicatedAcceleration, COND_SimulatedOnly);
}

void AFPSPlayerCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);
	
	if (UCharacterMovementComponent* MovementComponent = FPSMovementComponent)
	{
		// Compress Acceleration: XY components as direction + magnitude, Z component as direct value
		const double MaxAccel = MovementComponent->MaxAcceleration;
		const FVector CurrentAccel = MovementComponent->GetCurrentAcceleration();
		double AccelXYRadians, AccelXYMagnitude;
		FMath::CartesianToPolar(CurrentAccel.X, CurrentAccel.Y, AccelXYMagnitude, AccelXYRadians);

		ReplicatedAcceleration.AccelXYRadians   = FMath::FloorToInt((AccelXYRadians / TWO_PI) * 255.0);     // [0, 2PI] -> [0, 255]
		ReplicatedAcceleration.AccelXYMagnitude = FMath::FloorToInt((AccelXYMagnitude / MaxAccel) * 255.0);	// [0, MaxAccel] -> [0, 255]
		ReplicatedAcceleration.AccelZ           = FMath::FloorToInt((CurrentAccel.Z / MaxAccel) * 127.0);   // [-MaxAccel, MaxAccel] -> [-127, 127]
	}
}

void AFPSPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	if (AController* CharacterController = GetController())
	{
		if (CharacterController->IsPlayerController())
		{
			// --- Player-controlled character setup ---

			// Get the custom PlayerState class.
			AFPSPlayerState* PS = GetPlayerState<AFPSPlayerState>();
			
			// In multiplayer, only the locally controlled character on each client will have a valid PlayerController pointer.
			// For other (non-local) characters on that client, PlayerController will be nullptr. This is expected.
			AFPSPlayerController* PC = Cast<AFPSPlayerController>(CharacterController);
			if (PC)
			{
				// Get the custom HUD and initialize it with all gameplay references.
				AFPSHUD* FPSHUD = Cast<AFPSHUD>(PC->GetHUD());
				if (FPSHUD)
				{
					FPSHUD->InitializeOverlayWidget(PC, PS);
				}
			}
		}
	}
	
	InitializeOverheadWidget();
}

void AFPSPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	if (AController* CharacterController = GetController())
	{
		if (CharacterController->IsPlayerController())
		{
			// --- Player-controlled character setup ---

			// Get the custom PlayerState class.
			AFPSPlayerState* PS = GetPlayerState<AFPSPlayerState>();
			
			// In multiplayer, only the locally controlled character on each client will have a valid PlayerController pointer.
			// For other (non-local) characters on that client, PlayerController will be nullptr. This is expected.
			AFPSPlayerController* PC = Cast<AFPSPlayerController>(CharacterController);
			if (PC)
			{
				// Get the custom HUD and initialize it with all gameplay references.
				AFPSHUD* FPSHUD = Cast<AFPSHUD>(PC->GetHUD());
				if (FPSHUD)
				{
					FPSHUD->InitializeOverlayWidget(PC, PS);
				}
			}
		}
	}
	
	InitializeOverheadWidget();
	
	UpdateGait();
}

void AFPSPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// FORCIBLY SEVER THE WIDGET REFERENCE
	// This prevents the TransBuffer (Editor Undo History) from keeping the widget alive 
	// and causing a Fatal World Leak when PIE stops.
	if (OverheadWidgetComponent)
	{
		OverheadWidgetComponent->SetWidget(nullptr);
	}
	
	Super::EndPlay(EndPlayReason);
}

void AFPSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// =========================================================
	// LINK ANIMATION LAYERS & INPUT
	// =========================================================
	if (IsLocallyControlled())
	{
		
		if (APlayerController* PC = GetController<APlayerController>())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			{
				if (PlayerCharacterMappingContext) Subsystem->AddMappingContext(PlayerCharacterMappingContext, 0);
			}
		}
		
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			if (IFPSAnimInterface* FPSAnimInterface = Cast<IFPSAnimInterface>(AnimInstance))
			{
				FPSAnimInterface->SetFPSMode(CombatComponent && CombatComponent->GetEquippedWeapon());
			}
		}
	}

	if (DefaultAnimLayerClass && GetMesh())
	{
		GetMesh()->LinkAnimClassLayers(DefaultAnimLayerClass);
	}
	
	UpdateGait();
}

void AFPSPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	DeltaSeconds = DeltaTime;
    
	// Keep your PackedControlRotation logic inside "if (HasAuthority())" strictly!
	if (HasAuthority())
	{
		if (GetController())
		{
			FRotator CurrentRot = GetController()->GetControlRotation();
			ReplicatedControlRotation = UCharacterMovementComponent::PackYawAndPitchTo32(CurrentRot.Yaw, CurrentRot.Pitch);
		}
	}
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
	bWantsToWalk = true;
	UpdateGait(); // Central Resolver
}

void AFPSPlayerCharacter::OnWalkReleased()
{
	bWantsToWalk = false;
	UpdateGait();
}

void AFPSPlayerCharacter::OnSprintPressed()
{
	bWantsToSprint = true;
	UpdateGait();
}
void AFPSPlayerCharacter::OnSprintReleased()
{
	bWantsToSprint = false;
	UpdateGait();
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
	// 2. PERMISSION
	// Now that Gait is 'Running', CanFire() will pass (assuming we have ammo).
	if (CanFire())
	{
		// 1. INTERRUPT: Sprint-Out
		// If we are sprinting, force the state to Running immediately.
		StopSprinting(); // 1. Handle Interrupts
		CombatComponent->StartFire();
	}
}

void AFPSPlayerCharacter::OnFireReleased()
{
	if (!CombatComponent || !CombatComponent->GetEquippedWeapon()) return;
	
	CombatComponent->StopFire();
	if (bWantsToSprint) TryStartSprinting(); // Resume!
}

void AFPSPlayerCharacter::OnReloadPressed()
{
	// 2. PERMISSION
	if (CanReload())
	{
		// 1. INTERRUPT: Sprint-Out
		// If we are sprinting, force the state to Running immediately.
		StopSprinting(); // 1. Handle Interrupts
		CombatComponent->Reload();
	}
}

void AFPSPlayerCharacter::OnAimPressed()
{
	if (CanAim())
	{
		// 1. INTERRUPT: Sprint-Out
		// If we are sprinting, force the state to Running immediately.
		StopSprinting(); // 1. Handle Interrupts
		
		//CameraComponent->FieldOfView = FMath::FInterpTo(CameraComponent->FieldOfView, 60.f, DeltaSeconds, 10.f);
		SetAimState(EAimState::EAS_ADS);
		OnAimStateChanged.Broadcast(LayerStates.AimState);
	}
}

void AFPSPlayerCharacter::OnAimReleased()
{
	if (!CombatComponent || !CombatComponent->GetEquippedWeapon()) return;
	
	SetAimState(EAimState::EAS_None);
	//CameraComponent->FieldOfView = FMath::FInterpTo(CameraComponent->FieldOfView, 90.f, DeltaSeconds, 5.f);
	if (bWantsToSprint) TryStartSprinting(); // Resume!
	OnAimStateChanged.Broadcast(EAimState::EAS_None);
}

void AFPSPlayerCharacter::SetAimState(EAimState NewState)
{
	// 1. Prediction: Update locally immediately so it feels responsive
	LayerStates.AimState = NewState;
	OnAimStateChanged.Broadcast(LayerStates.AimState);
	
	if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
	{
		// Cast the object directly to the Interface (using the 'I' prefix, not 'U')
		if (IFPSAnimInterface* FPSAnimInterface = Cast<IFPSAnimInterface>(AnimInst))
		{
			// Now you can call the functions natively like any normal C++ object!
			bool bIsADS = (NewState == EAimState::EAS_ADS);
			FPSAnimInterface->SetUnarmed(false);
			FPSAnimInterface->SetADS(bIsADS);

		}
	}
	
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
	if (!FPSMovementComponent) return;
	
	// 1. Prediction (Client Visuals)
	LayerStates.Gait = NewState;
	OnGaitStateChanged.Broadcast(NewState);
	
	FWeaponMovementData MovementData;
	/*MovementData.AnimRunRefSpeed = FPSMovementComponent->GetRunSpeed();
	MovementData.AnimWalkRefSpeed = FPSMovementComponent->GetWalkSpeed();
	MovementData.AnimSprintRefSpeed = FPSMovementComponent->GetSprintSpeed();*/
	FPSMovementComponent->UpdateMovementSettings(MovementData);

	// 3. Replication (Server)
	if (!HasAuthority())
	{
		Server_SetGaitState(NewState);
	}
}

void AFPSPlayerCharacter::Server_SetGaitState_Implementation(EGait NewState)
{	
	if (!FPSMovementComponent) return;
	
	// 1. Update State
	LayerStates.Gait = NewState;
	
	FWeaponMovementData MovementData;
	/*MovementData.AnimRunRefSpeed = FPSMovementComponent->GetRunSpeed();
	MovementData.AnimWalkRefSpeed = FPSMovementComponent->GetWalkSpeed();
	MovementData.AnimSprintRefSpeed = FPSMovementComponent->GetSprintSpeed();*/
	FPSMovementComponent->UpdateMovementSettings(MovementData);
    
	// 2. Update Physics Speed (CRITICAL for preventing Rubber Banding)
	if (NewState == EGait::EG_Sprinting)
		FPSMovementComponent->MaxWalkSpeed = FPSMovementComponent->GetSprintSpeed();
	else if (NewState == EGait::EG_Walking)
		FPSMovementComponent->MaxWalkSpeed = FPSMovementComponent->GetWalkSpeed();
	else
		FPSMovementComponent->MaxWalkSpeed = FPSMovementComponent->GetRunSpeed();
}



void AFPSPlayerCharacter::SetOverlayState(EOverlayState NewState)
{
	LayerStates.OverlayState = NewState;
	OnOverlayStateChanged.Broadcast(NewState);
}

void AFPSPlayerCharacter::OnRep_LayerStates()
{
	// This runs on Simulated Proxies when the Server updates the state.
	// We use this to keep the visual speed sync logic in one place.
    
	// Ensure we apply the speed change to the Movement Component
	if (LayerStates.Gait == EGait::EG_Sprinting)
	{
		FPSMovementComponent->MaxWalkSpeed = FPSMovementComponent->GetSprintSpeed();
	}
	else if (LayerStates.Gait == EGait::EG_Walking)
	{
		FPSMovementComponent->MaxWalkSpeed = FPSMovementComponent->GetWalkSpeed();
	}
	else
	{
		FPSMovementComponent->MaxWalkSpeed = FPSMovementComponent->GetRunSpeed();
	}
    
	// Optional: Broadcast delegates if UI needs to know
	OnGaitStateChanged.Broadcast(LayerStates.Gait);
	OnOverlayStateChanged.Broadcast(LayerStates.OverlayState);
	OnAimStateChanged.Broadcast(LayerStates.AimState);
}

void AFPSPlayerCharacter::SetCurrentWeapon(AFPSWeapon* WeaponToEquip)
{
	// The Character knows about the component, so it delegates the work here.
	if (CombatComponent)
	{
		CombatComponent->EquipWeapon(WeaponToEquip);
	}
}

void AFPSPlayerCharacter::PlayFireMontage(UAnimMontage* HipFireMontage, UAnimMontage* AimedFireMontage)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;

	// DECISION LOGIC:
	// I am the Character. I know if I am aiming. The Weapon doesn't need to know.
	if (LayerStates.AimState == EAimState::EAS_ADS && AimedFireMontage)
	{
		AnimInstance->Montage_Play(AimedFireMontage);
	}
	else if (HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
	}
}

void AFPSPlayerCharacter::PlayEquipMontage(UAnimMontage* EquipMontage)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;
	
	if (CombatComponent->GetEquippedWeapon() && EquipMontage)
	{
		AnimInstance->Montage_Play(EquipMontage);
	}
}

void AFPSPlayerCharacter::PlayUnEquipMontage(UAnimMontage* UnEquipMontage)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;
	
	if (CombatComponent->GetEquippedWeapon() && UnEquipMontage)
	{
		AnimInstance->Montage_Play(UnEquipMontage);
	}
}

// =========================================================================
//                        STATE GATEKEEPERS (Rulesets)
// =========================================================================

bool AFPSPlayerCharacter::CanSprint() const
{
	if (!CombatComponent) return true;
	if (CombatComponent->GetCombatState() != ECombatState::ECS_Unoccupied) return false;
	if (FPSMovementComponent->IsFalling()) return false;
	return true;
}

bool AFPSPlayerCharacter::CanCrouch() const
{
	if (FPSMovementComponent->IsFalling()) return false;
	return Super::CanCrouch();
}

bool AFPSPlayerCharacter::CanAim() const
{
	// Safety First: If the component is missing, we definitely can't aim.
	if (!CombatComponent && !CombatComponent->GetEquippedWeapon()) return false;
	
	// STRICT RULE: Cannot aim if sprinting.
	if (LayerStates.Gait == EGait::EG_Sprinting) return false;
	
	if (CombatComponent->GetEquippedWeapon() == nullptr) return false;
	if (CombatComponent->GetCombatState() != ECombatState::ECS_Unoccupied) return false;
	return true;
}

bool AFPSPlayerCharacter::CanFire() const
{
	// Safety First: If the component is missing, we definitely can't fire.
	if (!CombatComponent || !CombatComponent->GetEquippedWeapon()) return false;
	
	// STRICT RULE: Cannot fire if sprinting.
	// Use the LayerState (Replicated), not the Component state, to ensure client/server agree.
	if (LayerStates.Gait == EGait::EG_Sprinting) return false;
	
	if (CombatComponent->GetEquippedWeapon() == nullptr) return false;
	if (CombatComponent->GetCombatState() != ECombatState::ECS_Unoccupied) return false;
	if (CombatComponent->GetEquippedWeapon()->GetCurrentClipAmmo() <= 0) return false;
	return true;
}

bool AFPSPlayerCharacter::CanReload() const
{
	// Safety First: If the component is missing, we definitely can't reload.
	if (!CombatComponent && !CombatComponent->GetEquippedWeapon()) return false;
	
	// STRICT RULE: Cannot reload if sprinting.
	if (LayerStates.Gait == EGait::EG_Sprinting) return false;
	
	if (CombatComponent->GetEquippedWeapon() == nullptr) return false;
	if (CombatComponent->GetCombatState() != ECombatState::ECS_Unoccupied) return false;
	if (CombatComponent->GetEquippedWeapon()->GetCurrentClipAmmo() >= CombatComponent->GetEquippedWeapon()->GetMaxClipAmmo()) return false;
	if (CombatComponent->GetCarriedAmmo() <= 0) return false;
	if (FPSMovementComponent->IsFalling()) return false;
	return true;
}

void AFPSPlayerCharacter::StopSprinting()
{
	if (LayerStates.Gait == EGait::EG_Sprinting)
	{
		FPSMovementComponent->MaxWalkSpeed = FPSMovementComponent->GetRunSpeed();
		SetGaitState(EGait::EG_Running);
	}
}

void AFPSPlayerCharacter::OnRep_ReplicatedAcceleration()
{
	if (FPSMovementComponent)
	{
		// Decompress Acceleration
		const double MaxAccel         = FPSMovementComponent->MaxAcceleration;
		const double AccelXYMagnitude = double(ReplicatedAcceleration.AccelXYMagnitude) * MaxAccel / 255.0; // [0, 255] -> [0, MaxAccel]
		const double AccelXYRadians   = double(ReplicatedAcceleration.AccelXYRadians) * TWO_PI / 255.0;     // [0, 255] -> [0, 2PI]

		FVector UnpackedAcceleration(FVector::ZeroVector);
		FMath::PolarToCartesian(AccelXYMagnitude, AccelXYRadians, UnpackedAcceleration.X, UnpackedAcceleration.Y);
		UnpackedAcceleration.Z = double(ReplicatedAcceleration.AccelZ) * MaxAccel / 127.0; // [-127, 127] -> [-MaxAccel, MaxAccel]

		FPSMovementComponent->SetReplicatedAcceleration(UnpackedAcceleration);
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
		FPSMovementComponent->MaxWalkSpeed = FPSMovementComponent->GetSprintSpeed();
		SetGaitState(EGait::EG_Sprinting); // This calls Server RPC internally!
	}
}

void AFPSPlayerCharacter::UpdateGait()
{
	// 1. Calculate Desired Gait based on Priority
	EGait DesiredGait = EGait::EG_Running; // Default

	if (bWantsToSprint && CanSprint())
	{
		DesiredGait = EGait::EG_Sprinting;
	}
	else if (bWantsToWalk)
	{
		DesiredGait = EGait::EG_Walking;
	}

	// 2. Only update if changed (Optimization)
	if (DesiredGait != LayerStates.Gait)
	{
		SetGaitState(DesiredGait);
	}
}

FRotator AFPSPlayerCharacter::GetReplicatedControlRotation() const
{
	// LOGIC FORK: Determine the source of truth based on who we are.
    
	// CASE A: We are the Owner (Autonomous Proxy)
	// I am playing this character on my machine.
	// I have direct access to the PlayerController input.
	// Return the raw, uncompressed input for maximum smoothness and responsiveness.
	if (IsLocallyControlled())
	{
		return GetControlRotation();
	}

	// CASE B: We are looking at someone else (Simulated Proxy)
	// This character is controlled by another player or the server.
	// I don't know their input. I must rely on the replicated data packet.
	// Decompress the 32-bit integer back into a Rotator
	FRotator DecompressedRot;
    
	// The Pitch is stored in the lower 16 bits
	uint16 PitchShort = (ReplicatedControlRotation & 0xFFFF);
    
	// The Yaw is stored in the upper 16 bits (Shift right by 16)
	uint16 YawShort = (ReplicatedControlRotation >> 16);

	// Use Unreal's native decompression
	DecompressedRot.Pitch = FRotator::DecompressAxisFromShort(PitchShort);
	DecompressedRot.Yaw   = FRotator::DecompressAxisFromShort(YawShort);
	DecompressedRot.Roll  = 0.f;

	return DecompressedRot;
}

void AFPSPlayerCharacter::InitializeOverheadWidget()
{
	// Assuming you have a WidgetComponent on your character for the name tag
	// Replace 'OverheadWidgetComponent' with whatever your component is actually named
	if (OverheadWidgetComponent)
	{
		if (UFPSPlayerInfoWidget* InfoWidget = Cast<UFPSPlayerInfoWidget>(OverheadWidgetComponent->GetUserWidgetObject()))
		{
			// Now we are 100% sure GetPlayerState() is valid!
			InfoWidget->SetPlayerState(GetPlayerState(), true);
			OnOverheadWidgetInitialized();
		}
	}
}