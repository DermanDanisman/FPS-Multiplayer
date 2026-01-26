// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "UI/PlayerInfoWidget.h"

void UPlayerInfoWidget::SetPlayerState(APlayerState* InPlayerState, bool bIs3DWorldWidget)
{
	// 1. Cleanup Old Bindings
	// If this widget is being recycled (ListView), it might still be listening to the old player.
	if (MyPlayerState)
	{
		if (auto* OldComp = MyPlayerState->FindComponentByClass<UPlayerIdentityComponent>())
		{
			OldComp->OnAvatarLoaded.RemoveAll(this);
		}
	}
	
	MyPlayerState = InPlayerState;
	bIsIn3DWorld = bIs3DWorldWidget;

	if (MyPlayerState)
	{
		// 2. DYNAMIC COMPONENT LOOKUP
		// We use FindComponentByClass because the PlayerState could be ALobbyPlayerState OR AMP_PlayerState.
		// As long as it has the component, this logic works.
		UPlayerIdentityComponent* IdentityComp = MyPlayerState->FindComponentByClass<UPlayerIdentityComponent>();
		
		if (IdentityComp)
		{
			// Bind Event: Wait for texture
			IdentityComp->OnAvatarLoaded.AddDynamic(this, &ThisClass::OnAvatarLoaded);

			// Trigger Fetch (Component handles the "Wait for Valid ID" timer)
			IdentityComp->FetchSteamAvatar();
			
			// Immediate Update: If the texture is already cached, show it now.
			if (IdentityComp->AvatarTexture)
			{
				OnAvatarLoaded(IdentityComp->AvatarTexture);
			}
		}

		// Update Name Text immediately
		UpdateVisuals(); 
	}
}

void UPlayerInfoWidget::OnAvatarLoaded(UTexture2D* AvatarTexture)
{
	// Trigger Blueprint to set the Brush
	UpdateVisuals();
}