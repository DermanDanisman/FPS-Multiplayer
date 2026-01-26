// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "LobbyPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "LobbyGameState.h"
#include "LobbyPlayerState.h"
#include "Blueprint/UserWidget.h"
#include "Subsystem/PlayerPersistenceSubsystem.h"
#include "UI/LobbyMenu.h"

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// 1. SETUP INPUT
	if (const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
	{
		if (auto* InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if(DefaultMappingContext) InputSystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// 2. SYNC PERSISTENCE (Local Only)
	// Pull data from GameInstance (MainMenu selections) and push to PlayerState.
	if (IsLocalController())
	{
		UGameInstance* GI = GetGameInstance();
		if (auto* Subsystem = GI->GetSubsystem<UPlayerPersistenceSubsystem>())
		{
			if (ALobbyPlayerState* LobbyPS = GetPlayerState<ALobbyPlayerState>())
			{
				// Push Cosmetics only (Team is selected in Lobby)
				if (Subsystem->LocalPlayerData.SkinId != 0)
				{
					LobbyPS->SetSkin(Subsystem->LocalPlayerData.SkinId);
					UE_LOG(LogTemp, Warning, TEXT("LobbyController: Pushed Skin %d from Main Menu"), Subsystem->LocalPlayerData.SkinId);
				}
			}
		}
	}
	
	TryCreateLobbyMenu();
}

void ALobbyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);

	if (IA_OpenChat)
	{
		EnhancedInputComponent->BindAction(IA_OpenChat, ETriggerEvent::Triggered, this, &ThisClass::OpenChat);
	}
}

void ALobbyPlayerController::OpenChat(const FInputActionValue& InputActionValue)
{
	// Signal UI to Focus Chat Box
	OnEnterPressedDelegate.Broadcast();
}

void ALobbyPlayerController::PopulatePersistentData(int32 NewTeamId)
{
	if (ALobbyPlayerState* LobbyPS = GetPlayerState<ALobbyPlayerState>())
	{
		LobbyPS->SetTeam(NewTeamId);
	}
}

void ALobbyPlayerController::UpdateLobbySettings(const FLobbySettings& NewSettings)
{
	Server_UpdateLobbySettings(NewSettings);
}

void ALobbyPlayerController::Server_UpdateLobbySettings_Implementation(const FLobbySettings& NewSettings)
{
	// SECURITY: Only Host (Authority) can change settings.
	if (!HasAuthority()) return; 

	if (ALobbyGameState* GS = GetWorld()->GetGameState<ALobbyGameState>())
	{
		GS->UpdateLobbySettings(NewSettings);
	}
}

void ALobbyPlayerController::TryCreateLobbyMenu()
{
	if (!IsValid(LobbyMenuClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("LobbyController: No LobbyMenuClass set. Skipping."));
		return;
	}

	// 1. Safety Check: Only create if local and not already created
	if (LobbyMenuInstance == nullptr && IsLocalController())
	{
		LobbyMenuInstance = CreateWidget<ULobbyMenu>(this, LobbyMenuClass);
        
		if (IsValid(LobbyMenuInstance))
		{
			LobbyMenuInstance->MenuSetup();
		}
	}
}

void ALobbyPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	// Retry creating UI now that PlayerState is valid
	TryCreateLobbyMenu();
}