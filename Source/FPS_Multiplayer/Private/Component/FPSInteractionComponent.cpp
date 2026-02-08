// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "Component/FPSInteractionComponent.h"

#include "Interface/FPSInteractableInterface.h"
#include "Kismet/KismetSystemLibrary.h"

UFPSInteractionComponent::UFPSInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
void UFPSInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UFPSInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// Optimization: Don't scan every single frame. 10 times a second is plenty.
	if (GetWorld()->GetTimeSeconds() - LastInteractionCheckTime > InteractionCheckFrequency)
	{
		FindBestInteractable();
		LastInteractionCheckTime = GetWorld()->GetTimeSeconds();
	}
}

void UFPSInteractionComponent::FindBestInteractable()
{
	// 1. Safety Check: Only run on Controller/Client (Server doesn't need UI)
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled()) return;
	
	AActor* OldInteractable = CurrentInteractable;
	
	// 1. SPHERE OVERLAP (Find everything nearby)
	TArray<AActor*> OverlappingActors;
	UKismetSystemLibrary::SphereOverlapActors(
		this, 
		GetOwner()->GetActorLocation(), 
		InteractionDistance, 
		ObjectTypes, 
		nullptr, 
		TArray<AActor*>(), // Ignore
		OverlappingActors
	);
	
	// 2. FILTER & SORT (Find the specific one we are looking at)
	AActor* BestCandidate = nullptr;
	float BestDotProduct = -1.0f; // -1 = Behind us, 1 = Directly in front

	FVector CameraLocation; 
	FRotator CameraRotation;
	GetOwner()->GetActorEyesViewPoint(CameraLocation, CameraRotation);
	FVector CameraForward = CameraRotation.Vector();
	
	// --- DEBUG CONFIG ---
	if (bShowDebug)
	{
		// Draw the interaction range
		DrawDebugSphere(GetWorld(), GetOwner()->GetActorLocation(), InteractionDistance, 12, FColor::Red, false, -1.0f, 0, 1.0f);
	}

	for (AActor* PotentialActor : OverlappingActors)
	{
		// Must implement the Interface!
		if (PotentialActor && PotentialActor->Implements<UFPSInteractableInterface>())
		{
			// Math: Calculate Dot Product to see if we are looking at it
			FVector DirectionToItem = (PotentialActor->GetActorLocation() - CameraLocation).GetSafeNormal();
			float Dot = FVector::DotProduct(CameraForward, DirectionToItem);
			
			// --- DEBUG VISUALIZATION ---
			if (bShowDebug)
			{
				// Color Logic: Green = Good candidate, Yellow = Too far angle, Red = Behind us
				FColor DebugColor = FColor::Red;
				if (Dot > 0.5f) DebugColor = FColor::Green;
				else if (Dot > 0.0f) DebugColor = FColor::Yellow;

				// Draw Line to Item
				DrawDebugLine(GetWorld(), CameraLocation, PotentialActor->GetActorLocation(), DebugColor, false, -1.0f, 0, 1.0f);
				
				// Draw Score Text
				FString DebugText = FString::Printf(TEXT("Dot: %.2f"), Dot);
				DrawDebugString(GetWorld(), PotentialActor->GetActorLocation(), DebugText, nullptr, FColor::White, 0.0f);
			}

			// "0.5" means roughly 45 degrees cone in front of player
			if (Dot > 0.5f && Dot > BestDotProduct)
			{
				BestDotProduct = Dot;
				BestCandidate = PotentialActor;
			}
		}
	}
	
	// 3. HANDLE STATE CHANGES (Show/Hide Widgets)
	if (BestCandidate != CurrentInteractable)
	{
		// A. We were looking at something, now we are looking at something else (or nothing)
		if (CurrentInteractable) // Ensure variable is valid
		{
			if (CurrentInteractable->Implements<UFPSInteractableInterface>())
			{
				// Hide the "Press E" widget on the OLD item
				IFPSInteractableInterface::Execute_OnFocusLost(CurrentInteractable, OwnerPawn);
			}
		}

		// B. We found a NEW item
		if (BestCandidate)
		{
			if (BestCandidate->Implements<UFPSInteractableInterface>())
			{
				// Show the "Press E" widget on the NEW item
				IFPSInteractableInterface::Execute_OnFocusGained(BestCandidate, OwnerPawn);
				
				// Highlight the WINNER
				if (bShowDebug)
				{
					DrawDebugSphere(GetWorld(), BestCandidate->GetActorLocation(), 30.0f, 12, FColor::Cyan, false, 0.5f);
				}
			}
		}

		// C. Update the pointer
		CurrentInteractable = BestCandidate;
	}
}

void UFPSInteractionComponent::PrimaryInteract()
{
	if (CurrentInteractable)
	{
		// EXECUTE INTERFACE
		IFPSInteractableInterface::Execute_Interact(CurrentInteractable, Cast<APawn>(GetOwner()));
	}
}


