// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "Component/PlayerIdentityComponent.h"
#include "GameFramework/PlayerState.h"

UPlayerIdentityComponent::UPlayerIdentityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerIdentityComponent::FetchSteamAvatar()
{
	// 1. Get ID
	FUniqueNetIdRepl PlayerID = GetOwnerUniqueId();

	// 2. SAFETY CHECK: Is the ID valid yet?
	// When a client joins, the PlayerState exists, but the ID might still be replicating.
	if (!PlayerID.IsValid())
	{
		// Retry logic: Wait 1.0 second and try again.
		FTimerHandle RetryHandle;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(RetryHandle, this, &ThisClass::FetchSteamAvatar, 1.0f, false);
		}
		return;
	}

	// 3. FETCH
	AvatarTexture = UOnlineMultiplayerSessionLibrary::GetSteamAvatar(PlayerID, PreferredAvatarSize);

	// 4. BROADCAST
	if (AvatarTexture)
	{
		OnAvatarLoaded.Broadcast(AvatarTexture);
	}
	else
	{
		// If ID is valid but Steam returned null (downloading...), retry fast (0.5s).
		FTimerHandle RetryHandle;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(RetryHandle, this, &ThisClass::FetchSteamAvatar, 0.5f, false);
		}
	}
}

FUniqueNetIdRepl UPlayerIdentityComponent::GetOwnerUniqueId() const
{
	if (APlayerState* PS = Cast<APlayerState>(GetOwner()))
	{
		return PS->GetUniqueId();
	}
	return FUniqueNetIdRepl();
}