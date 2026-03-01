// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "AnimModifiers/TurnYawAnimModifier.h"

#include "AnimPose.h"
#include "AnimTools.h"

FTransform UTurnYawAnimModifier::ExtractBoneTransform(const UAnimSequence* AnimationSequence,
	const FBoneContainer& BoneContainer, FCompactPoseBoneIndex CompactPoseBoneIndex, float Time, bool bComponentSpace)
{
	ensureAlways(!AnimationSequence->bForceRootLock);

	FCompactPose Pose;
	Pose.SetBoneContainer(&BoneContainer);

	FBlendedCurve Curve;
	Curve.InitFrom(BoneContainer);

	const FAnimExtractContext Context(static_cast<double>(Time), false);
	UE::Anim::FStackAttributeContainer Attributes;
	FAnimationPoseData AnimationPoseData(Pose, Curve, Attributes);

	AnimationSequence->GetBonePose(AnimationPoseData, Context, true);

	check(Pose.IsValidIndex(CompactPoseBoneIndex));

	if (bComponentSpace)
	{
		FCSPose<FCompactPose> ComponentSpacePose;
		ComponentSpacePose.InitPose(Pose);
		return ComponentSpacePose.GetComponentSpaceTransform(CompactPoseBoneIndex);
	}
	else
	{
		return Pose[CompactPoseBoneIndex];
	}
}

void UTurnYawAnimModifier::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
	if (AnimationSequence == nullptr)
	{
		UE_LOG(LogAnimation, Error, TEXT("MotionExtractorModifier failed. Reason: Invalid AnimationSequence"));
		return;
	}

	USkeleton* Skeleton = AnimationSequence->GetSkeleton();
	if (Skeleton == nullptr)
	{
		UE_LOG(LogAnimation, Error, TEXT("MotionExtractorModifier failed. Reason: AnimationSequence with invalid Skeleton. AnimationSequence: %s"),
			*GetNameSafe(AnimationSequence));
		return;
	}

	const int32 BoneIndex = Skeleton->GetReferenceSkeleton().FindBoneIndex(BoneName);
	if (BoneIndex == INDEX_NONE)
	{
		UE_LOG(LogAnimation, Error, TEXT("MotionExtractorModifier failed. Reason: Invalid Bone Index. BoneName: %s AnimationSequence: %s Skeleton: %s"),
			*BoneName.ToString(), *GetNameSafe(AnimationSequence), *GetNameSafe(Skeleton));
		return;
	}

	FMemMark Mark(FMemStack::Get());

	TGuardValue<bool> ForceRootLockGuard(AnimationSequence->bForceRootLock, false);

	TArray<FBoneIndexType> RequiredBones;
	RequiredBones.Add(BoneIndex);
	Skeleton->GetReferenceSkeleton().EnsureParentsExistAndSort(RequiredBones);
	
	UE::Anim::FCurveFilterSettings FilterSettings;
	const FBoneContainer BoneContainer(RequiredBones, FilterSettings, *Skeleton);
	
	const FCompactPoseBoneIndex CompactPoseBoneIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(BoneIndex));

	static constexpr bool bComponentSpace = true;
	const FTransform LastFrameBoneTransform = ExtractBoneTransform(AnimationSequence, BoneContainer, CompactPoseBoneIndex, AnimationSequence->GetPlayLength(), bComponentSpace);

	const float AnimLength = AnimationSequence->GetPlayLength();
	const float SampleInterval = 1.f / SampleRate;

	float Time = 0.f;
	int32 SampleIndex = 0;

	TArray<FRichCurveKey> CurveKeys;
	TArray<FRichCurveKey> WeightCurveKeys;

	// First weight key
	bool bPlacedWeightEndKey = false;
	{
		FRichCurveKey& Key = WeightCurveKeys.AddDefaulted_GetRef();
		Key.Time = 0.f;
		Key.Value = 1.f;
		Key.InterpMode = RCIM_Constant;
	}

	while (Time < AnimLength)
	{
		Time = FMath::Clamp(SampleIndex * SampleInterval, 0.f, AnimLength);
		const float NextTime = FMath::Clamp((SampleIndex + 1) * SampleInterval, 0.f, AnimLength);
		SampleIndex++;

		FTransform BoneTransform = ExtractBoneTransform(AnimationSequence, BoneContainer, CompactPoseBoneIndex, Time, bComponentSpace);

		static constexpr bool bRelativeToFirstFrame = true;
		if (bRelativeToFirstFrame)
		{
			BoneTransform = BoneTransform.GetRelativeTransform(LastFrameBoneTransform).Inverse();
		}
		
		const float Value = BoneTransform.GetRotation().Rotator().Yaw;
		const float Pct = Time / AnimLength;
		const bool bMaxPct = Pct >= (1.f - MaxWeightOffsetPct);
		const bool bMaxBlendTime = NextTime >= (AnimLength - GraphTransitionBlendTime);  // Start the frame before the transition
		if (!bPlacedWeightEndKey && (FMath::IsNearlyZero(Value, 0.1f) || bMaxBlendTime || bMaxPct))
		{
			// Guess where the turn actually ends
			bPlacedWeightEndKey = true;
			FRichCurveKey& Key = WeightCurveKeys.AddDefaulted_GetRef();
			Key.Time = Time;
			Key.Value = 0.f;
			Key.InterpMode = RCIM_Constant;
		}

		FRichCurveKey& Key = CurveKeys.AddDefaulted_GetRef();
		Key.Time = Time;
		Key.Value = Value;
	}

	// Remove curve if it exists
	if (UAnimationBlueprintLibrary::DoesCurveExist(AnimationSequence, TurnYawCurveName, ERawCurveTrackTypes::RCT_Float))
	{
		UAnimationBlueprintLibrary::RemoveCurve(AnimationSequence, TurnYawCurveName);
	}

	if (UAnimationBlueprintLibrary::DoesCurveExist(AnimationSequence, TurnWeightCurveName, ERawCurveTrackTypes::RCT_Float))
	{
		UAnimationBlueprintLibrary::RemoveCurve(AnimationSequence, TurnWeightCurveName);
	}

	IAnimationDataController& Controller = AnimationSequence->GetController();
	// Remaining Turn Yaw
	{
		const FAnimationCurveIdentifier CurveId = UAnimationCurveIdentifierExtensions::GetCurveIdentifier(AnimationSequence->GetSkeleton(), TurnYawCurveName, ERawCurveTrackTypes::RCT_Float);
		if (CurveKeys.Num() && Controller.AddCurve(CurveId))
		{
			Controller.SetCurveKeys(CurveId, CurveKeys);
		}
	}

	// Weight
	const FAnimationCurveIdentifier CurveId = UAnimationCurveIdentifierExtensions::GetCurveIdentifier(AnimationSequence->GetSkeleton(), TurnWeightCurveName, ERawCurveTrackTypes::RCT_Float);
	if (WeightCurveKeys.Num() && Controller.AddCurve(CurveId))
	{
		Controller.SetCurveKeys(CurveId, WeightCurveKeys);
	}
}

void UTurnYawAnimModifier::OnRevert_Implementation(UAnimSequence* AnimationSequenceSequence)
{
	Super::OnRevert_Implementation(AnimationSequenceSequence);
	
	UAnimationBlueprintLibrary::RemoveCurve(AnimationSequenceSequence, TurnYawCurveName);
	UAnimationBlueprintLibrary::RemoveCurve(AnimationSequenceSequence, TurnWeightCurveName);
}
