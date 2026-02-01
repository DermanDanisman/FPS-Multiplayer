// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "Player/FPSPlayerState.h"

#include "Component/FPSChatComponent.h"
#include "Component/FPSPlayerIdentityComponent.h"

AFPSPlayerState::AFPSPlayerState()
{
	IdentityComponent = CreateDefaultSubobject<UFPSPlayerIdentityComponent>("IdentityComponent");
	ChatComponent = CreateDefaultSubobject<UFPSChatComponent>("ChatComponent");
	
	// CRITICAL: Components on PlayerState MUST replicate to allow Server->Client RPCs.
	ChatComponent->SetIsReplicated(true);
}

void AFPSPlayerState::BeginPlay()
{
	Super::BeginPlay();
	
	// Auto-fetch Steam Avatar when this player spawns.
	if (IdentityComponent)
	{
		IdentityComponent->FetchSteamAvatar();
	}
}
