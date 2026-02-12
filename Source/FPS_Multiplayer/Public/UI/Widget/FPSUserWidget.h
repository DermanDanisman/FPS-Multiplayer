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

	virtual void SetWidgetController(UObject* InWidgetController);
};
