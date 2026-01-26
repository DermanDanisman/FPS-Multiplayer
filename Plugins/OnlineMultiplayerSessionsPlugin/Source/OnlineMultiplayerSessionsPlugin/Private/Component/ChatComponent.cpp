// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "Component/ChatComponent.h"
#include "LobbyGameState.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

UChatComponent::UChatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UChatComponent::SendChatMessage(const FString& MessageText, EChatMessageType Type)
{
	if (MessageText.IsEmpty()) return;
	
	// Client -> Server
	Server_SendChatMessage(MessageText, Type);
}

void UChatComponent::Server_SendChatMessage_Implementation(const FString& MessageText, EChatMessageType Type)
{
	// 1. Construct Message
	FChatMessage NewMessage;
	NewMessage.MessageText = MessageText;
	NewMessage.MessageType = Type;
	
	// 2. Authoritative Sender Name
	// Never trust the client to send their own name. Get it from the PlayerState we own.
	if (APlayerState* PS = Cast<APlayerState>(GetOwner()))
	{
		NewMessage.SenderName = PS->GetPlayerName();
	}
	
	// 3. Handoff to GameState
	// The GameState handles adding it to history and broadcasting to all clients.
	ALobbyGameState* LobbyGS = Cast<ALobbyGameState>(UGameplayStatics::GetGameState(GetOwner()));
	if (LobbyGS)
	{
		LobbyGS->Multicast_BroadcastChatMessage(NewMessage);
	}
}

bool UChatComponent::Server_SendChatMessage_Validate(const FString& MessageText, EChatMessageType Type)
{
	// Anti-Spam / Buffer Overflow Protection
	if (MessageText.Len() > 256) return false; // Kick player if they send huge text
	if (MessageText.IsEmpty()) return false;
	
	return true;
}