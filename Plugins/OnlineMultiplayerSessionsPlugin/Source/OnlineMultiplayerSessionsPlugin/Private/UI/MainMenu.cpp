// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "UI/MainMenu.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Subsystem/PlayerPersistenceSubsystem.h"
#include "UI/HostSession.h"
#include "UI/ServerBrowser.h"
#include "Components/PanelWidget.h" // Required to access AddChild/ClearChildren

void UMainMenu::NativeConstruct()
{
	Super::NativeConstruct();
	
	// --- DEBUG / TESTING ---
	// Demonstration of how to save data before a map transition.
	UGameInstance* GI = GetGameInstance();
	if (UPlayerPersistenceSubsystem* Subsystem = GI->GetSubsystem<UPlayerPersistenceSubsystem>())
	{
		// Simulate user picking a skin (e.g. "Alien" = ID 5).
		Subsystem->LocalPlayerData.SkinId = 5; 
		UE_LOG(LogTemp, Warning, TEXT("TEST: MainMenu set SkinId to 5"));
	}
	
	// --- BINDINGS ---
	if (Button_Host) Button_Host->OnClicked.AddDynamic(this, &ThisClass::OnHostClicked);
	if (Button_Search) Button_Search->OnClicked.AddDynamic(this, &ThisClass::OnSearchClicked);
	if (Button_Quit) Button_Quit->OnClicked.AddDynamic(this, &ThisClass::OnQuitClicked);
}

void UMainMenu::OnHostClicked()
{
	// 1. Validation
	if (!HostSessionClass || !ServerBrowserContainer) return;

	// 2. Create the Sub-Widget
	// We cast to UHostSession so we can bind to its specific delegate.
	UHostSession* HostWidget = CreateWidget<UHostSession>(GetWorld(), HostSessionClass);
	if (HostWidget)
	{
		// 3. Bind Return Delegate
		// When the user clicks "Back" in the Host menu, it calls OnChildWidgetClosed.
		HostWidget->OnHostSessionBack.AddDynamic(this, &ThisClass::OnChildWidgetClosed);

		// 4. Inject
		ServerBrowserContainer->ClearChildren(); // Remove any previous menu
		ServerBrowserContainer->AddChild(HostWidget);

		// 5. Hide Main Navigation
		// We hide the main buttons so the user focuses on the new form.
		Button_Host->SetVisibility(ESlateVisibility::Collapsed);
		Button_Search->SetVisibility(ESlateVisibility::Collapsed);
		Button_Quit->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UMainMenu::OnSearchClicked()
{
	if (!ServerBrowserClass || !ServerBrowserContainer) return;

	// Same logic as HostClicked, but for the Server Browser.
	UServerBrowser* Browser = CreateWidget<UServerBrowser>(GetWorld(), ServerBrowserClass);
	
	if (Browser)
	{
		Browser->OnServerBrowserBack.AddDynamic(this, &ThisClass::OnChildWidgetClosed);

		ServerBrowserContainer->ClearChildren();
		ServerBrowserContainer->AddChild(Browser);
        
		Button_Host->SetVisibility(ESlateVisibility::Collapsed);
		Button_Search->SetVisibility(ESlateVisibility::Collapsed);
		Button_Quit->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UMainMenu::OnQuitClicked()
{
	// Hard Quit.
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, true);
}

void UMainMenu::OnChildWidgetClosed()
{
	// RESET STATE:
	// 1. Show Main Buttons again.
	Button_Host->SetVisibility(ESlateVisibility::Visible);
	Button_Search->SetVisibility(ESlateVisibility::Visible);
	Button_Quit->SetVisibility(ESlateVisibility::Visible);
    
	// 2. Clear the container (destroys the sub-widget).
	if (ServerBrowserContainer)
	{
		ServerBrowserContainer->ClearChildren();
	}
}
