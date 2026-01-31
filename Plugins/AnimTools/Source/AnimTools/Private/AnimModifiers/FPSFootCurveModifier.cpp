// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimModifiers/FPSFootCurveModifier.h"
#include "AnimPose.h"
#include "Animation/AnimSequence.h"
#include "AnimationBlueprintLibrary.h"

void UFPSFootCurveModifier::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
	if (!AnimationSequence) return;

	// 1. Clean Start: Remove the curve if it already exists so we don't get duplicate data.
	if (UAnimationBlueprintLibrary::DoesCurveExist(AnimationSequence, CurveName, ERawCurveTrackTypes::RCT_Float))
	{
		UAnimationBlueprintLibrary::RemoveCurve(AnimationSequence, CurveName);
	}
	UAnimationBlueprintLibrary::AddCurve(AnimationSequence, CurveName);

	// 2. Configure the Evaluation Options
	// We want "Raw" data to see exactly what the animation file contains.
	FAnimPoseEvaluationOptions EvalOptions;
	EvalOptions.EvaluationType = EAnimDataEvalType::Raw;
	EvalOptions.bExtractRootMotion = true; // Important! Calculate positions assuming root moves.
	EvalOptions.bEvaluateCurves = false;
	
	int32 NumFrames;
	UAnimationBlueprintLibrary::GetNumFrames(AnimationSequence, NumFrames);
	
	// --- STEP 3: PROCESS FRAMES ---
	for (int32 FrameIndex = 0; FrameIndex <= NumFrames; ++FrameIndex)
	{
		// Get Timestamp
		float Time;
		UAnimationBlueprintLibrary::GetTimeAtFrame(AnimationSequence, FrameIndex, Time);

		// A. READ DATA (AnimPoseExtensions)
		FAnimPose Pose;
		UAnimPoseExtensions::GetAnimPoseAtTime(AnimationSequence, static_cast<double>(Time), EvalOptions, Pose);

		// Get Transforms in "World" space (which means Component Space / Actor Space here)
		FTransform BoneTransform = UAnimPoseExtensions::GetBonePose(Pose, BoneName, EAnimPoseSpaces::World);
		FTransform RootTransform = UAnimPoseExtensions::GetBonePose(Pose, FName("root"), EAnimPoseSpaces::World);

		// Calculate Distance (Delta)
		FVector Delta = BoneTransform.GetLocation() - RootTransform.GetLocation();
       
		float CurveValue = 0.f;
		switch (AxisToTrack)
		{
		case EAxis::X: CurveValue = Delta.X; break;
		case EAxis::Y: CurveValue = Delta.Y; break;
		case EAxis::Z: CurveValue = Delta.Z; break;
		default: ;
		}

		// B. WRITE DATA (AnimationBlueprintLibrary)
		UAnimationBlueprintLibrary::AddFloatCurveKey(AnimationSequence, CurveName, Time, CurveValue);
	}
    
	UE_LOG(LogTemp, Log, TEXT("Generated Curve %s for %s"), *CurveName.ToString(), *AnimationSequence->GetName());
}

void UFPSFootCurveModifier::OnRevert_Implementation(UAnimSequence* AnimationSequence)
{
	if (AnimationSequence)
	{
		UAnimationBlueprintLibrary::RemoveCurve(AnimationSequence, CurveName);
	}
}