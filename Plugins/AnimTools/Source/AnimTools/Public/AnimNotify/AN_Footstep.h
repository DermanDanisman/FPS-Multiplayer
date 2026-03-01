// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_Footstep.generated.h"

class UFootstepDataAsset;
/**
 * A modular animation notify that fires a line trace to detect the physical 
 * surface type beneath the character's foot and plays the appropriate sound/FX.
 */
UCLASS()
class ANIMTOOLS_API UAN_Footstep : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	
	UAN_Footstep();
	
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	
	// Optional: Overriding this gives your notify a clean name in the editor timeline
	virtual FString GetNotifyName_Implementation() const override;
	
	/**
	 * The name of the bone to trace from. 
	 * You will set this to "foot_r" or "foot_l" in the animation editor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep settings")
	FName FootBoneName;
	
	// NEW: We replaced the TMap with a single pointer to our database
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep Settings")
	TObjectPtr<UFootstepDataAsset> FootstepData;
	
protected:

	/** How far down from the foot bone should we trace? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep settings")
	float TraceLength = 50.0f;
    
	/** If true, draws a debug line showing where the foot traced. */
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDrawDebug = false;
	
};
