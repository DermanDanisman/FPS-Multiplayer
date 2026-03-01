// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Chaos/ChaosEngineInterface.h" // Required for EPhysicalSurface
#include "FootstepDataAsset.generated.h"

class USoundBase;
class UNiagaraSystem; // UE5 standard for VFX. Use UParticleSystem if using Cascade.

/**
 * A bundle containing the audio and visual effects for a specific surface.
 */
USTRUCT(BlueprintType)
struct FFootstepEffects
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TObjectPtr<USoundBase> Sound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TObjectPtr<UNiagaraSystem> Particle = nullptr;
};

/**
 * WHAT: A centralized database mapping physics surfaces to their effects.
 * WHY: Allows audio designers to update footsteps globally in one place without 
 * touching animations or code.
 */
UCLASS(BlueprintType)
class ANIMTOOLS_API UFootstepDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:

	/** The fallback effects if we hit an unmapped surface. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footsteps")
	FFootstepEffects DefaultEffects;

	/** The dictionary linking physical surfaces to their specific effects. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Footsteps")
	TMap<TEnumAsByte<EPhysicalSurface>, FFootstepEffects> SurfaceEffects;
};
