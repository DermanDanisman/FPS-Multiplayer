// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/Enums/FPSCharacterTypes.h"
#include "FPSCharacterDataContainer.generated.h"

USTRUCT(BlueprintType)
struct FCharacterLayerStates
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "Layer States")
	EStance Stance = EStance::ES_Standing;

	UPROPERTY(BlueprintReadOnly, Category = "Layer States")
	EGait Gait = EGait::EG_Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Layer States")
	EActionState ActionState = EActionState::EAS_None;

	UPROPERTY(BlueprintReadOnly, Category = "Layer States")
	EOverlayState OverlayState = EOverlayState::EOS_Unarmed;
	
	UPROPERTY(BlueprintReadOnly, Category = "Layer States")
	EAimState AimState = EAimState::EAS_None;
};

USTRUCT(BlueprintType)
struct FLocomotionAnimCardinalDirections
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion Cardinal Directions")
	TObjectPtr<UAnimSequence> Forward;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion Cardinal Directions")
	TObjectPtr<UAnimSequence> Backward;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion Cardinal Directions")
	TObjectPtr<UAnimSequence> Left;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion Cardinal Directions")
	TObjectPtr<UAnimSequence> Right;
};