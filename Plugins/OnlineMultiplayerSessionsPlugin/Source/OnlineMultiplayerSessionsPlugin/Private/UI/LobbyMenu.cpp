// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "UI/LobbyMenu.h"

#include "LobbyGameMode.h"
#include "LobbyGameState.h"
#include "LobbyPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Utility/OnlineMultiplayerSessionLibrary.h"

void ULobbyMenu::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 1. Host vs Client Visibility Logic
	if (GetOwningPlayer()->HasAuthority())
	{
		// HOST: Sees "Start", Hides "Ready" (Auto-Ready)
		if (Button_Invite) Button_Invite->SetVisibility(ESlateVisibility::Visible);
		if (Button_Start) Button_Start->SetVisibility(ESlateVisibility::Visible);
		if (Button_Ready) Button_Ready->SetVisibility(ESlateVisibility::Collapsed);
        
		// Start a timer to constantly check "Are we all ready?"
		FTimerHandle StartButtonCheckHandle;
		GetWorld()->GetTimerManager().SetTimer(StartButtonCheckHandle, this, &ThisClass::CheckStartButton, 0.5f, true);
	}
	else
	{
		// CLIENT: Sees "Ready", Hides "Start"
		if (Button_Invite) Button_Invite->SetVisibility(ESlateVisibility::Collapsed);
		if (Button_Start) Button_Start->SetVisibility(ESlateVisibility::Collapsed);
		if (Button_Ready) Button_Ready->SetVisibility(ESlateVisibility::Visible);
	}

	// 2. Bind Buttons
	if (Button_Invite) Button_Invite->OnClicked.AddDynamic(this, &ThisClass::OnInviteClicked);
	if (Button_Ready) Button_Ready->OnClicked.AddDynamic(this, &ThisClass::OnReadyClicked);
	if (Button_Start) Button_Start->OnClicked.AddDynamic(this, &ThisClass::OnStartClicked);
	
	// 3. Start Init Loop
	InitLobby();
}

void ULobbyMenu::InitLobby()
{
	ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(UGameplayStatics::GetGameState(GetOwningPlayer()));
	ALobbyPlayerState* LobbyPS = Cast<ALobbyPlayerState>(GetOwningPlayer()->PlayerState);

	// GATEKEEPER: If pointers are null (replication delay), retry in 0.2s.
	if (!IsValid(LobbyGS) || !IsValid(LobbyPS))
	{
		FTimerHandle RetryHandle;
		GetWorld()->GetTimerManager().SetTimer(RetryHandle, this, &ThisClass::InitLobby, 0.2f, false);
		return;
	}

	// Ready! Set state.
	SetPlayerState(LobbyPS);
	
	// --- BINDING THE RESTORED EVENTS ---
	
	// 1. Player List Logic
	// When GameState says "Player Joined/Left", we call the BP Event "OnPlayerListChanged".
	LobbyGS->OnPlayerListChangedDelegate.RemoveAll(this);
	LobbyGS->OnPlayerListChangedDelegate.AddDynamic(this, &ThisClass::OnPlayerListChanged);
	
	// Force initial refresh
	OnPlayerListChanged();

	// 2. Local Player Data
	LobbyPS->OnPersistentDataSet.RemoveAll(this);
	LobbyPS->OnPersistentDataSet.AddDynamic(this, &ThisClass::OnPersistentDataUpdated);
    
	// 3. Ready Status Logic
	// When our ready status changes, update the button or play sound via BP.
	LobbyPS->OnReadyChanged.RemoveAll(this);
	LobbyPS->OnReadyChanged.AddDynamic(this, &ThisClass::OnReadyStatusChanged);

	// Initial Sync
	OnPersistentDataUpdated(LobbyPS->PersistenceData);
	OnReadyStatusChanged(LobbyPS->bIsReady);
}

void ULobbyMenu::OnInviteClicked()
{
	// Opens Steam Overlay
	UOnlineMultiplayerSessionLibrary::OpenSteamFriendsUI();
}

void ULobbyMenu::OnReadyClicked()
{
	ALobbyPlayerState* LobbyPS = Cast<ALobbyPlayerState>(GetOwningPlayer()->PlayerState);
	if (!IsValid(LobbyPS)) return;
	
	// Toggle Ready State (Client -> Server RPC handled internally by PS)
	LobbyPS->SetIsReady(!LobbyPS->bIsReady);
}

void ULobbyMenu::CheckStartButton()
{
	// Host Only
	if (!GetOwningPlayer()->HasAuthority()) return;

	ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(UGameplayStatics::GetGameState(GetOwningPlayer()));
	if (LobbyGS)
	{
		bool bAllReady = true;

		// Iterate all players
		for (APlayerState* PS : LobbyGS->PlayerArray)
		{
			ALobbyPlayerState* LobbyPS = Cast<ALobbyPlayerState>(PS);
			if (!IsValid(LobbyPS)) continue;
            
			// If anyone is NOT ready, we cannot start.
			if (!LobbyPS->bIsReady)
			{
				bAllReady = false;
				break; 
			}
		}

		// Enable/Disable Start Button
		if (Button_Start) Button_Start->SetIsEnabled(bAllReady);
	}
}

void ULobbyMenu::OnStartClicked()
{
	// Tell GameMode to Travel
	if (ALobbyGameMode* GM = Cast<ALobbyGameMode>(UGameplayStatics::GetGameMode(GetOwningPlayer())))
	{
		GM->StartGame();
	}
}

void ULobbyMenu::SetPlayerState(ALobbyPlayerState* InPlayerState)
{
	MyPlayerState = InPlayerState;
}