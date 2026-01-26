// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "UI/PasswordPrompt.h"

void UPasswordPrompt::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (Button_Join) Button_Join->OnClicked.AddDynamic(this, &ThisClass::OnJoinClicked);
	if (Button_Cancel) Button_Cancel->OnClicked.AddDynamic(this, &ThisClass::OnCancelClicked);
}

void UPasswordPrompt::OnJoinClicked()
{
	if (Input_Password)
	{
		// Broadcast: Success = True, Data = Text
		OnPasswordResult.Broadcast(Input_Password->GetText().ToString(), true);
	}
	// Close Self
	RemoveFromParent();
}

void UPasswordPrompt::OnCancelClicked()
{
	// Broadcast: Success = False
	OnPasswordResult.Broadcast(FString(), false);
	RemoveFromParent();
}