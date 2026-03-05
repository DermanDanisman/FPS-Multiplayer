// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSCameraComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class FPS_MULTIPLAYER_API UFPSCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UFPSCameraComponent();
	
	UFUNCTION(BlueprintCallable, Category = "FPS Camera Component")
	void SetCameraMode(UAnimInstance* AnimInstance, bool bFPSCamera);
	
};
