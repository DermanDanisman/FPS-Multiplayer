// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utility/OnlineMultiplayerSessionLibrary.h"
#include "PlayerIdentityComponent.generated.h"

// Delegate: Fires when the texture is successfully downloaded and created.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSteamAvatarLoaded, UTexture2D*, AvatarTexture);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API UPlayerIdentityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerIdentityComponent();
	
	// Config: Preferred size (Small/Medium/Large). Can be set in Blueprint.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	ESteamAvatarSize PreferredAvatarSize = ESteamAvatarSize::Small;
	
	// Cache: Holds the texture once loaded.
	UPROPERTY(BlueprintReadOnly, Category = "Identity")
	TObjectPtr<UTexture2D> AvatarTexture;

	// Event: UI binds to this to update the Image Brush.
	UPROPERTY(BlueprintAssignable, Category = "Identity")
	FOnSteamAvatarLoaded OnAvatarLoaded;

	// API: Call this to start the fetch process (handles retries automatically).
	UFUNCTION(BlueprintCallable, Category = "Identity")
	void FetchSteamAvatar();
	
	// Helper to get ID from owner PlayerState.
	FUniqueNetIdRepl GetOwnerUniqueId() const;
};