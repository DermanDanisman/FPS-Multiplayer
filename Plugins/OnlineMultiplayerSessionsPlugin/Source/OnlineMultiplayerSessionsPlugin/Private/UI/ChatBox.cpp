// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "UI/ChatBox.h"

#include "LobbyGameState.h"
#include "LobbyPlayerController.h"
#include "Components/ScrollBox.h"
#include "Kismet/GameplayStatics.h"

void UChatBox::NativeConstruct()
{
	Super::NativeConstruct();
	
	// Start the setup process immediately
	InitChatBox();
}

void UChatBox::InitChatBox()
{
	ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(UGameplayStatics::GetGameState(GetOwningPlayer()));
	ALobbyPlayerController* LobbyPC = Cast<ALobbyPlayerController>(GetOwningPlayer());

	// 1. THE GATEKEEPER PATTERN
	// When joining a server, the UI might spawn 1-2 frames BEFORE the GameState
	// has replicated to this machine. Pointers will be null.
	// If null, we set a timer to try again in 0.2 seconds.
	if (!IsValid(LobbyGS) || !IsValid(LobbyPC))
	{
		FTimerHandle RetryHandle;
		GetWorld()->GetTimerManager().SetTimer(RetryHandle, this, &ThisClass::InitChatBox, 0.2f, false);
		return;
	}

	// 2. CLEANUP & BINDING
	// Remove old delegates to prevent double-firing if InitChatBox runs multiple times.
	LobbyGS->OnChatMessageReceived.RemoveAll(this);
	LobbyGS->OnChatMessageReceived.AddDynamic(this, &ThisClass::OnChatMessageReceived);
	
	LobbyPC->OnEnterPressedDelegate.RemoveAll(this);
	LobbyPC->OnEnterPressedDelegate.AddDynamic(this, &ThisClass::OnEnterPressed);

	// 3. HISTORY REPLAY (Critical for Late Joiners)
	// The GameState holds an array of messages sent before we joined.
	// We loop through them and display them now so the chat isn't empty.
	if (ScrollBox_Chat)
	{
		ScrollBox_Chat->ClearChildren(); // Clear any dummy text
		
		for (const FChatMessage& OldMsg : LobbyGS->ChatHistory)
		{
			// Call the BP event to spawn the widget row
			OnChatMessageReceived(OldMsg); 
		}
	}
}