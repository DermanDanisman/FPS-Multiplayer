// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FPSAnimInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UFPSAnimInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class FPS_MULTIPLAYER_API IFPSAnimInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	
	virtual void SetFPSMode(bool bInFPSMode) = 0;
	virtual bool GetFPSMode() = 0;
	virtual void SetUnarmed(bool bInUnarmed) = 0;
	virtual void SetADS(bool bInADS) = 0;
	virtual void SetADSUpper(bool bInADSUpper) = 0;
	virtual float GetCameraPitchWeight() = 0;
	virtual void SetFPSPelvisWeight(float InPelvisWeight) = 0;
};
