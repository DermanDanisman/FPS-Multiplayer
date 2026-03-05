// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "Component/FPSCameraComponent.h"

#include "Interface/FPSAnimInterface.h"


UFPSCameraComponent::UFPSCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UFPSCameraComponent::SetCameraMode(UAnimInstance* AnimInstance, bool bFPSCamera)
{
	IFPSAnimInterface* AnimInterface = Cast<IFPSAnimInterface>(AnimInstance);
	if (AnimInterface)
	{
		AnimInterface->SetFPSMode(bFPSCamera);
	}
}


