// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineMultiplayerSessionTypes.h"
#include "Components/ActorComponent.h"
#include "ChatComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API UChatComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UChatComponent();

	// Called by UI to send text.
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SendChatMessage(const FString& MessageText, EChatMessageType Type = EChatMessageType::Global);

protected:
	
	// Server RPC: Reliable (Must arrive), WithValidation (Check constraints).
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendChatMessage(const FString& MessageText, EChatMessageType Type);
};