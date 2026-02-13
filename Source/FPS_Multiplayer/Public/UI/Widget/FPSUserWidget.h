// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSUserWidget.generated.h"

/**
 * 
 */
UCLASS()
class FPS_MULTIPLAYER_API UFPSUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	
	/**
	 * Sets the controller and triggers the Blueprint event.
	 * Use this instead of manually setting the variable.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetWidgetController(UObject* InWidgetController);

	/** 
	 * The Controller (ViewModel) holding the data.
	 * Marked BlueprintReadOnly so the Designer can bind to it.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UObject> WidgetController;

protected:
	
	/** 
	 * Event called immediately after the Widget Controller is set.
	 * USE THIS in Blueprint to Cast the Controller and Bind your variables!
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void WidgetControllerSet();
};
