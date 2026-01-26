// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "Utility/OnlineMultiplayerSessionLibrary.h"

#include "HttpModule.h"
#include "OnlineMultiplayerSessionTypes.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "Interfaces/IHttpResponse.h"

// -----------------------------------------------------------------------------
// STEAM API INCLUSION
// -----------------------------------------------------------------------------
// Dedicated Servers do not link against steam_api64.dll. 
// We must protect this include or the server build will fail.
#if !UE_SERVER
	#pragma push_macro("fold")
	#undef fold
	#include <steam/steam_api.h>
	#pragma pop_macro("fold")
#endif

// -----------------------------------------------------------------------------
// INTERNAL HELPERS (Hidden from Header)
// -----------------------------------------------------------------------------
namespace SteamAvatarHelpers
{
#if !UE_SERVER
	// Helper: Converts Unreal's "Steam:12345" string ID to a raw uint64 that Steam understands.
	uint64 GetSteamID64(const FUniqueNetIdRepl& UniqueId)
	{
		if (!UniqueId.IsValid()) return 0;
		FString IdStr = UniqueId->ToString();
		
		// Strip the subsystem prefix if present
		if (IdStr.StartsWith("Steam:")) IdStr = IdStr.RightChop(6);
		
		return FCString::Strtoui64(*IdStr, nullptr, 10);
	}

	// Helper: Converts raw pixel data from Steam to a UTexture2D
	UTexture2D* CreateTextureFromSteamHandle(int32 SteamImageHandle)
	{
		if (!SteamUtils()) return nullptr;
		
		uint32 Width = 0, Height = 0;
		if (!SteamUtils()->GetImageSize(SteamImageHandle, &Width, &Height)) return nullptr;
		
		// Allocate buffer
		TArray<uint8> RawData;
		RawData.SetNum(Width * Height * 4); // 4 channels (RGBA)
		
		if (!SteamUtils()->GetImageRGBA(SteamImageHandle, RawData.GetData(), RawData.Num())) return nullptr;
		
		// --- PIXEL FORMAT SWAP ---
		// Steam gives RGBA. Unreal expects BGRA. We must swap channels.
		for (uint32 i = 0; i < (Width * Height); ++i)
		{
			const int32 Index = i * 4;
			uint8 TempR = RawData[Index + 0];
			RawData[Index + 0] = RawData[Index + 2]; // Move Blue to Red
			RawData[Index + 2] = TempR;              // Move Red to Blue
		}
		
		// Create Transient Texture
		UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
		if (NewTexture)
		{
			// Lock memory, copy data, unlock
			void* TextureData = NewTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(TextureData, RawData.GetData(), RawData.Num());
			NewTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
			NewTexture->UpdateResource();
		}

		return NewTexture;
	}
#endif
}

// -----------------------------------------------------------------------------
// LIBRARY IMPLEMENTATION
// -----------------------------------------------------------------------------

UTexture2D* UOnlineMultiplayerSessionLibrary::GetSteamAvatar(const FUniqueNetIdRepl& UniqueId, ESteamAvatarSize Size)
{
#if !UE_SERVER
	if (!SteamAPI_IsSteamRunning()) return nullptr;

	uint64 SteamID64 = SteamAvatarHelpers::GetSteamID64(UniqueId);
	if (SteamID64 == 0) return nullptr;

	// Ask Steam for the Handle
	int32 Handle = 0;
	CSteamID SteamIDObj(SteamID64);

	switch (Size)
	{
		case ESteamAvatarSize::Small:  Handle = SteamFriends()->GetSmallFriendAvatar(SteamIDObj); break;
		case ESteamAvatarSize::Medium: Handle = SteamFriends()->GetMediumFriendAvatar(SteamIDObj); break;
		case ESteamAvatarSize::Large:  Handle = SteamFriends()->GetLargeFriendAvatar(SteamIDObj); break;
	}

	// -1 means "Waiting for download". 0 means "Invalid". >0 means "Ready".
	if (Handle > 0)
	{
		return SteamAvatarHelpers::CreateTextureFromSteamHandle(Handle);
	}
#endif
	// If server, or steam not running, or avatar not downloaded yet
	return nullptr;
}

UTexture2D* UOnlineMultiplayerSessionLibrary::GetLocalSteamAvatar(ESteamAvatarSize Size)
{
#if !UE_SERVER
	if (SteamUser())
	{
		CSteamID LocalID = SteamUser()->GetSteamID();
		// Convert CSteamID to uint64, then to string, then wrap in UniqueId for our helper
		// (Or just duplicate logic, but let's assume we use the helper above if we had the ID)
		// For simplicity in this snippet, we'll just check handles directly:
		
		int32 Handle = 0;
		switch (Size)
		{
			case ESteamAvatarSize::Small:  Handle = SteamFriends()->GetSmallFriendAvatar(LocalID); break;
			case ESteamAvatarSize::Medium: Handle = SteamFriends()->GetMediumFriendAvatar(LocalID); break;
			case ESteamAvatarSize::Large:  Handle = SteamFriends()->GetLargeFriendAvatar(LocalID); break;
		}
		if (Handle > 0) return SteamAvatarHelpers::CreateTextureFromSteamHandle(Handle);
	}
#endif
	return nullptr;
}

void UOnlineMultiplayerSessionLibrary::OpenSteamFriendsUI()
{
#if !UE_SERVER
	if (SteamFriends())
	{
		SteamFriends()->ActivateGameOverlay("friends");
	}
#endif
}

FString UOnlineMultiplayerSessionLibrary::GetCustomSessionName(const FOnlineSessionSearchResult& SearchResult)
{
	FString ServerName = "Unknown Server";
	if (SearchResult.Session.SessionSettings.Settings.Contains(SETTING_SERVER_NAME))
	{
		SearchResult.Session.SessionSettings.Settings[SETTING_SERVER_NAME].Data.GetValue(ServerName);
	}
	return ServerName;
}

int32 UOnlineMultiplayerSessionLibrary::GetCurrentPlayerCount(const FOnlineSessionSearchResult& SearchResult)
{
	// Math: Total Slots - Available Slots = Used Slots
	const int32 MaxPublic = SearchResult.Session.SessionSettings.NumPublicConnections;
	const int32 OpenPublic = SearchResult.Session.NumOpenPublicConnections;
	
	// Note: We ignore private slots usually for lobby listings, but here is the full math:
	// const int32 MaxPrivate = SearchResult.Session.SessionSettings.NumPrivateConnections;
	// const int32 OpenPrivate = SearchResult.Session.NumOpenPrivateConnections;
	
	return MaxPublic - OpenPublic;
}

FString UOnlineMultiplayerSessionLibrary::GetHostName(const FOnlineSessionSearchResult& SearchResult)
{
	return SearchResult.Session.OwningUserName;
}

void UOnlineMultiplayerSessionLibrary::CheckInternetConnection(const FString& TestURL)
{
	FHttpModule* Http = &FHttpModule::Get();
	if (!Http) return;

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	Request->SetURL(TestURL);
	Request->SetVerb("HEAD"); // Lightweight ping
    
	Request->OnProcessRequestComplete().BindLambda([](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
	{
		if (bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
		{
			// Connection Good
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Internet Connection: OK"));
		}
		else
		{
			// Connection Bad
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Internet Connection: FAILED"));
		}
	});

	Request->ProcessRequest();
}

const UOnlineMultiplayerSessionsDeveloperSettings* UOnlineMultiplayerSessionLibrary::GetOnlineMultiplayerSessionsDeveloperSettings()
{
	return GetDefault<UOnlineMultiplayerSessionsDeveloperSettings>();
}

UTexture2D* UOnlineMultiplayerSessionLibrary::LoadSoftTexture(TSoftObjectPtr<UTexture2D> SoftTexture)
{
	if (SoftTexture.IsNull()) return nullptr;
	return SoftTexture.LoadSynchronous(); // Force load from disk
}