// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "AnimNotify/AN_Footstep.h"

#include "NiagaraFunctionLibrary.h"
#include "Data/FootstepDataAsset.h"
#include "Kismet/GameplayStatics.h"

UAN_Footstep::UAN_Footstep()
{
	FootBoneName = NAME_None;
    
#if WITH_EDITORONLY_DATA
	// This colors the notify block in the animation timeline (Light Blue)
	NotifyColor = FColor(100, 200, 255, 255);
#endif
}

FString UAN_Footstep::GetNotifyName_Implementation() const
{
	// If you haven't assigned a bone yet, show a warning. 
	// Otherwise, show "Step: foot_r" on the timeline.
	if (FootBoneName == NAME_None)
	{
		return TEXT("Step: NO BONE");
	}
    
	return FString::Printf(TEXT("Step: %s"), *FootBoneName.ToString());
}

void UAN_Footstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	// ALWAYS call Super first so the engine can route its internal logic.
    Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp || FootBoneName == NAME_None) return;

    UWorld* World = MeshComp->GetWorld();
    if (!World) return;

    // 1. Get the exact world location of the foot bone
    FVector BoneLocation = MeshComp->GetSocketLocation(FootBoneName);

    // 2. Setup the Line Trace (Raycast) straight down
    FVector TraceStart = BoneLocation;
    FVector TraceEnd = TraceStart - FVector(0.f, 0.f, TraceLength);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    
    // CRITICAL: We MUST tell the trace to return the Physical Material, or our surface switch won't work!
    QueryParams.bReturnPhysicalMaterial = true; 
    
    // Ignore the character's own collision capsule so we actually hit the floor
    QueryParams.AddIgnoredActor(MeshComp->GetOwner());

    // 3. Fire the Trace
    bool bHit = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

    // 4. Start by assuming we will use the Default Effects
	FFootstepEffects EffectsToPlay = FootstepData->DefaultEffects;

	// 5. If we hit the ground, check the Data Asset for a specific surface match
	if (bHit && HitResult.PhysMaterial.IsValid())
	{
		EPhysicalSurface SurfaceType = HitResult.PhysMaterial->SurfaceType;

		// Query the Data Asset's TMap
		if (const FFootstepEffects* FoundEffects = FootstepData->SurfaceEffects.Find(SurfaceType))
		{
			// If we found a match (e.g., Grass), overwrite our default effects
			EffectsToPlay = *FoundEffects;
		}
	}

	// 6. Play the Audio (if assigned)
	if (EffectsToPlay.Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, EffectsToPlay.Sound, BoneLocation);
	}

	// 7. Play the Particle VFX (if assigned)
	if (EffectsToPlay.Particle)
	{
		// Spawns the dust/sparks exactly where the raycast hit the floor
		FVector SpawnLocation = bHit ? HitResult.Location : BoneLocation;
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, EffectsToPlay.Particle, SpawnLocation);
	}

    // 6. Debugging (Editor Only)
#if !UE_BUILD_SHIPPING
    if (bDrawDebug)
    {
        FColor LineColor = bHit ? FColor::Green : FColor::Red;
        DrawDebugLine(World, TraceStart, TraceEnd, LineColor, false, 2.0f, 0, 1.0f);
        if (bHit)
        {
            DrawDebugSphere(World, HitResult.Location, 5.0f, 12, FColor::Yellow, false, 2.0f);
        }
    }
#endif
}
