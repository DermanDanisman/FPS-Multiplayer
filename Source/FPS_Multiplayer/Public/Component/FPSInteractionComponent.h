// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSInteractionComponent.generated.h"

// Forward declaration
class IFPSInteractableInterface;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class FPS_MULTIPLAYER_API UFPSInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UFPSInteractionComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// The Input Action calls this
	void PrimaryInteract();

protected:
	
	// Config: Which object types can we interact with?
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;

	// Config: How far away can we interact?
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionDistance = 200.0f;
    
	// Config: How often to scan for items (0.1s is fine, per-frame is expensive)
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionCheckFrequency = 0.1f;
	
	virtual void BeginPlay() override;
	
private:
	// The current "Best" item we would interact with if we pressed E
	UPROPERTY()
	TObjectPtr<AActor> CurrentInteractable;

	// Helper to find the best target
	void FindBestInteractable();
    
	float LastInteractionCheckTime;
	
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowDebug = false; // Set this to false to disable
};
