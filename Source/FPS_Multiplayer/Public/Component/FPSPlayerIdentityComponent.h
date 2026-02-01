// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Component/PlayerIdentityComponent.h"
#include "FPSPlayerIdentityComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPS_MULTIPLAYER_API UFPSPlayerIdentityComponent : public UPlayerIdentityComponent
{
	GENERATED_BODY()

public:

	UFPSPlayerIdentityComponent();

protected:

	virtual void BeginPlay() override;
	
};
