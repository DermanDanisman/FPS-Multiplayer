// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Component/ChatComponent.h"

#include "FPSChatComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPS_MULTIPLAYER_API UFPSChatComponent : public UChatComponent
{
	GENERATED_BODY()

public:

	UFPSChatComponent();

protected:

	virtual void BeginPlay() override;
};
