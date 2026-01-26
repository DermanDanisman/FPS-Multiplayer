// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineMultiplayerSessionTypes.h"
#include "Blueprint/UserWidget.h"
#include "ChatBox.generated.h"

class UScrollBox;

/**
 * @class UChatBox
 * @brief The UI Widget that displays the chat history and input box.
 * * ARCHITECTURE:
 * This widget does not handle networking directly.
 * It listens to the LobbyGameState for incoming messages.
 * It listens to the LobbyPlayerController for input (Enter Key).
 */
UCLASS()
class ONLINEMULTIPLAYERSESSIONSPLUGIN_API UChatBox : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	virtual void NativeConstruct() override;
	
	// --- BLUEPRINT EVENTS ---
	
	// Fired when a message arrives (Real-time or History).
	// BP Implementation: Create a 'ChatRow' widget and add it to the ScrollBox.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Chat UI")
	void OnChatMessageReceived(const FChatMessage& NewMessage);
	
	// Fired when the User presses "Enter".
	// BP Implementation: Set Focus to the EditableTextBox.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Chat UI")
	void OnEnterPressed();
	
protected:
	
	// The container for the messages.
	// Must be named "ScrollBox_Chat" in the Widget Hierarchy.
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category = "Chat UI")
	TObjectPtr<UScrollBox> ScrollBox_Chat;
	
	// The initialization loop that handles replication delay.
	void InitChatBox();
};