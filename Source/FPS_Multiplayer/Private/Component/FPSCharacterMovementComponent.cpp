// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "Component/FPSCharacterMovementComponent.h"

#include "Character/FPSPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Data/DataAsset/FPSWeaponData.h"
#include "GameFramework/Character.h"

namespace FPSPlayerCharacter
{
	static float GroundTraceDistance = 100000.0f;
	FAutoConsoleVariableRef CVar_GroundTraceDistance(TEXT("LyraCharacter.GroundTraceDistance"), GroundTraceDistance, TEXT("Distance to trace down when generating ground information."), ECVF_Cheat);
}

UFPSCharacterMovementComponent::UFPSCharacterMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFPSCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

const FCharacterGroundInfo& UFPSCharacterMovementComponent::GetGroundInfo()
{
	if (!CharacterOwner || (GFrameCounter == CachedGroundInfo.LastUpdateFrame))
	{
		return CachedGroundInfo;
	}

	if (MovementMode == MOVE_Walking)
	{
		CachedGroundInfo.GroundHitResult = CurrentFloor.HitResult;
		CachedGroundInfo.GroundDistance = 0.0f;
	}
	else
	{
		const UCapsuleComponent* CapsuleComp = CharacterOwner->GetCapsuleComponent();
		check(CapsuleComp);

		const float CapsuleHalfHeight = CapsuleComp->GetUnscaledCapsuleHalfHeight();
		const ECollisionChannel CollisionChannel = (UpdatedComponent ? UpdatedComponent->GetCollisionObjectType() : ECC_Pawn);
		const FVector TraceStart(GetActorLocation());
		const FVector TraceEnd(TraceStart.X, TraceStart.Y, (TraceStart.Z - FPSPlayerCharacter::GroundTraceDistance - CapsuleHalfHeight));

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LyraCharacterMovementComponent_GetGroundInfo), false, CharacterOwner);
		FCollisionResponseParams ResponseParam;
		InitCollisionParams(QueryParams, ResponseParam);

		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, CollisionChannel, QueryParams, ResponseParam);

		CachedGroundInfo.GroundHitResult = HitResult;
		CachedGroundInfo.GroundDistance = FPSPlayerCharacter::GroundTraceDistance;

		if (MovementMode == MOVE_NavWalking)
		{
			CachedGroundInfo.GroundDistance = 0.0f;
		}
		else if (HitResult.bBlockingHit)
		{
			CachedGroundInfo.GroundDistance = FMath::Max((HitResult.Distance - CapsuleHalfHeight), 0.0f);
		}
	}

	CachedGroundInfo.LastUpdateFrame = GFrameCounter;

	return CachedGroundInfo;
}

void UFPSCharacterMovementComponent::SetReplicatedAcceleration(const FVector& InAcceleration)
{
	bHasReplicatedAcceleration = true;
	
	Acceleration = InAcceleration;
}

void UFPSCharacterMovementComponent::UpdateMovementSettings(const FWeaponMovementData& NewData)
{
	AFPSPlayerCharacter* FPSCharacter = Cast<AFPSPlayerCharacter>(GetCharacterOwner());
	if (!FPSCharacter) return;
	
	/*// 1. Cache the values so we can swap back and forth
	RunSpeed = NewData.MaxBaseSpeed;
	WalkSpeed = NewData.AnimWalkRefSpeed;
	SprintSpeed = NewData.AnimSprintRefSpeed; // Assumes this exists in your struct
	MaxWalkSpeedCrouched = NewData.MaxCrouchSpeed;*/
	
	// 2. Apply the correct speed based on current state
	if (FPSCharacter->GetGaitState() == EGait::EG_Walking)
	{
		MaxWalkSpeed = WalkSpeed;
	}
	else if (FPSCharacter->GetGaitState() == EGait::EG_Sprinting)
	{
		MaxWalkSpeed = SprintSpeed;
	}
	else
	{
		MaxWalkSpeed = RunSpeed;
	}
}

