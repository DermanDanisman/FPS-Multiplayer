// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "UI/HostSession.h"

#include "OnlineMultiplayerSessionSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "Utility/OnlineMultiplayerSessionsDeveloperSettings.h"

void UHostSession::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 1. Button Bindings
	if (Button_Create) Button_Create->OnClicked.AddDynamic(this, &ThisClass::OnCreateClicked);
	if (Button_Back) Button_Back->OnClicked.AddDynamic(this, &ThisClass::OnBackClicked);
	
	// 2. Checkbox Logic
	if (Checkbox_Private)
	{
		Checkbox_Private->OnCheckStateChanged.AddDynamic(this, &ThisClass::OnPrivateFlagChanged);
		// Force update initial state (e.g. disable password box if starting unchecked)
		OnPrivateFlagChanged(Checkbox_Private->IsChecked());
	}
	
	// 3. Text Input Listeners (for validation)
	if (Input_ServerName) Input_ServerName->OnTextChanged.AddDynamic(this, &ThisClass::OnServerNameChanged);
	if (Input_Password) Input_Password->OnTextChanged.AddDynamic(this, &ThisClass::OnPasswordTextChanged);
	
	// 4. Initial Validation Check
	RefreshButtonState();
}

void UHostSession::RefreshButtonState()
{
	if (!Button_Create || !Input_ServerName || !Checkbox_Private || !Input_Password) return;

	// Rule: Server name cannot be empty.
	bool bValidName = !Input_ServerName->GetText().IsEmpty();
	
	// Rule: If Private, Password cannot be empty.
	bool bValidPassword = !Checkbox_Private->IsChecked() || !Input_Password->GetText().IsEmpty();

	Button_Create->SetIsEnabled(bValidName && bValidPassword);
}

void UHostSession::OnPrivateFlagChanged(bool bIsChecked)
{
	// UX: Disable password box if server is public.
	if (Input_Password)
	{
		Input_Password->SetIsEnabled(bIsChecked);
		if (!bIsChecked)
		{
			Input_Password->SetText(FText::GetEmpty());
		}
	}
	RefreshButtonState();
}

void UHostSession::OnServerNameChanged(const FText& Text)
{
	RefreshButtonState();
}

void UHostSession::OnPasswordTextChanged(const FText& Text)
{
	RefreshButtonState();
}

void UHostSession::OnCreateClicked()
{
	// Disable button to prevent double-click spam
	if (Button_Create) Button_Create->SetIsEnabled(false);

	// 1. Config Object
	FServerCreateSettings Settings;
	Settings.ServerName = Input_ServerName->GetText().ToString();
	Settings.bIsPrivate = Checkbox_Private->IsChecked();
	Settings.Password = Input_Password->GetText().ToString();
	Settings.bIsLAN = false; // Set to true for local testing without Steam
	Settings.MaxPlayers = 4;

	// 2. Call Subsystem
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		if (auto* Subsystem = GI->GetSubsystem<UOnlineMultiplayerSessionSubsystem>())
		{
			// Listen for completion
			Subsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSessionComplete);
			Subsystem->CreateServer(Settings);
		}
	}
}

void UHostSession::OnBackClicked()
{
	// Tell Parent to close us
	OnHostSessionBack.Broadcast();
	RemoveFromParent();
}

void UHostSession::OnCreateSessionComplete(bool bWasSuccessful)
{
	// 1. CLEANUP: Unbind delegate immediately to prevent memory leaks or ghost logic.
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		if (auto* Subsystem = GI->GetSubsystem<UOnlineMultiplayerSessionSubsystem>())
		{
			Subsystem->MultiplayerOnCreateSessionComplete.RemoveDynamic(this, &ThisClass::OnCreateSessionComplete);
		}
	}

	if (bWasSuccessful)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			// 2. GET MAP PATH: Read from Project Settings
			FString PathToTravel = "/Game/Maps/Lobby"; // Hardcoded Fallback

			const UOnlineMultiplayerSessionsDeveloperSettings* UserSettings = GetDefault<UOnlineMultiplayerSessionsDeveloperSettings>();
			if (UserSettings && !UserSettings->LobbyMapPath.IsNull())
			{
				PathToTravel = UserSettings->LobbyMapPath.GetLongPackageName();
			}

			// 3. SERVER TRAVEL: Open the Lobby map as a Listen Server.
			World->ServerTravel(FString::Printf(TEXT("%s?listen"), *PathToTravel));
		}
	}
	else
	{
		// 4. FAIL STATE: Re-enable button so user can try again.
		if (Button_Create) Button_Create->SetIsEnabled(true);
		
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Session Creation Failed!"));
	}
}