// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "Component/FPSChatComponent.h"

UFPSChatComponent::UFPSChatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFPSChatComponent::BeginPlay()
{
	Super::BeginPlay();
}
