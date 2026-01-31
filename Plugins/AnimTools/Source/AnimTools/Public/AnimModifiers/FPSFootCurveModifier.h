// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "FPSFootCurveModifier.generated.h"

/**
 * @class UFPSFootCurveModifier
 * @brief Generates a "Distance Curve" by tracking a bone's position relative to the root.
 * Essential for Distance Matching (Stops, Starts, Pivots).
 */
UCLASS()
class ANIMTOOLS_API UFPSFootCurveModifier : public UAnimationModifier
{
	GENERATED_BODY()
	
public:
	
	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;
	virtual void OnRevert_Implementation(UAnimSequence* AnimationSequence) override;
	
protected:
	
	// The bone we want to track (e.g., "foot_r")
	UPROPERTY(EditAnywhere, Category = "Settings")
	FName BoneName = FName("foot_r");
	
	// The name of the curve to generate (e.g., "FootPosition_Y")
	UPROPERTY(EditAnywhere, Category = "Settings")
	FName CurveName = FName("FootPosition_Y");
	
	// Which axis matters? (X = Forward/Back, Y = Right/Left, Z = Up/Down)
	// For "Stops", we usually care about X (Forward distance).
	UPROPERTY(EditAnywhere, Category = "Settings")
	TEnumAsByte<EAxis::Type> AxisToTrack = EAxis::Y;
};