// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "FPS_Multiplayer/Public/Animation/FPSAnimInstance.h"

#include "AnimCharacterMovementLibrary.h"
#include "TurnInPlace.h"
#include "TurnInPlaceStatics.h"
#include "Actor/Weapon/FPSWeapon.h"
#include "Camera/CameraComponent.h"
#include "Component/FPSCharacterMovementComponent.h"
#include "Component/FPSCombatComponent.h"
#include "FPS_Multiplayer/Public/Character/FPSPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

#pragma region Lifecycle

void UFPSAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	UpdateReferences();

	if (AActor* OwningActor = GetOwningActor())
	{
		WorldLocation = OwningActor->GetActorLocation();
	}
}

void UFPSAnimInstance::UpdateReferences()
{
	if (AFPSPlayerCharacter* Character = Cast<AFPSPlayerCharacter>(TryGetPawnOwner()))
	{
		UFPSCombatComponent* Combat = Character->GetCombatComponent();
		if (Combat)
		{
			Combat->OnWeaponEquipped.Remove(OnWeaponEquippedDelegateHandle);
			OnWeaponEquippedDelegateHandle = Combat->OnWeaponEquipped.AddUObject(
				this, &ThisClass::OnCharacterWeaponEquipped);

			if (Combat->GetEquippedWeapon())
			{
				OnCharacterWeaponEquipped(Combat->GetEquippedWeapon());
			}
		}
	}
}

void UFPSAnimInstance::NativeUninitializeAnimation()
{
	if (AFPSPlayerCharacter* Character = Cast<AFPSPlayerCharacter>(TryGetPawnOwner()))
	{
		if (UFPSCombatComponent* Combat = Character->GetCombatComponent())
		{
			Combat->OnWeaponEquipped.Remove(OnWeaponEquippedDelegateHandle);
		}
	}

	Super::NativeUninitializeAnimation();
}

// =========================================================================
//                  PHASE 1: GAME THREAD (Gather Data Safely)
// =========================================================================
void UFPSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	AFPSPlayerCharacter* Character = Cast<AFPSPlayerCharacter>(TryGetPawnOwner());
	if (!Character) return;

	UFPSCharacterMovementComponent* Movement = Cast<UFPSCharacterMovementComponent>(Character->GetMovementComponent());
	if (!Movement) return;

	// --- 1. GATHER RAW ACTOR DATA ---
	CachedActorLocation = Character->GetActorLocation();
	CachedActorRotation = Character->GetActorRotation();
	CachedVelocity = Character->GetVelocity();
	CachedAcceleration = Movement->GetCurrentAcceleration();
	if (Character->IsLocallyControlled())
	{
		CachedControlRotation = Character->GetControlRotation();
	}
	else
	{
		CachedControlRotation = Character->GetReplicatedControlRotation();
	}
	CachedLastUpdateVelocity = Movement->GetLastUpdateVelocity();
	
	// -------------------------------------------------------------------------
    // PROXY LOOK-AT TRACE (Zero Network Bandwidth Cost)
    // -------------------------------------------------------------------------
    // We only run this trace on other players (Proxies). Local players use standard IK.
    if (!Character->IsLocallyControlled())
    {
       // Reconstruct where the proxy's camera would be based on their head position
       FName SocketToUse = Character->GetMesh()->DoesSocketExist(FName("CameraSocket")) ? FName("CameraSocket") : HeadBoneName;
       FVector ProxyCameraLocation = Character->GetMesh()->GetSocketLocation(SocketToUse);
       
       // Create a forward vector using the pitch/yaw that was replicated over the network
       FVector ProxyCameraDirection = CachedControlRotation.Vector();

       FVector TraceStart = ProxyCameraLocation;
       FVector TraceEnd = TraceStart + (ProxyCameraDirection * 20000.f); // Trace out 200 meters

       FHitResult HitResult;
       FCollisionObjectQueryParams ObjectQueryParams;
       ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);       // Hit other players
       ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic); // Hit walls/floors
       
       FCollisionQueryParams Params;
       Params.AddIgnoredActor(Character); // Don't let the proxy hit their own body
       
       // Perform a LineTraceSingle on the Game Thread. 
       // This is cheap because it uses the physics spatial hash and only traces a single line.
       bool bHit = GetWorld()->LineTraceSingleByObjectType(HitResult, TraceStart, TraceEnd, ObjectQueryParams, Params);
       
       if (bHit)
       {
          // Cache the distance so the Worker Thread can read it for trigonometry
          CachedAimDistance = HitResult.Distance;
          
          if (bDrawProxyAimDebug)
          {
             DrawDebugLine(GetWorld(), TraceStart, HitResult.Location, FColor::Green, false, -1.f, 0, 1.f);
             DrawDebugSphere(GetWorld(), HitResult.Location, 8.f, 12, FColor::Red, false, -1.f);
          }
       }
       else
       {
          if (bDrawProxyAimDebug)
          {
             DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, -1.f, 0, 1.f);
          }
       }
    }

	// --- 2. GATHER MOVEMENT COMPONENT DATA ---
	CachedGravityZ = Movement->GetGravityZ();
	CachedMaxWalkSpeed = Movement->MaxWalkSpeed;
	CachedCurrentWalkSpeed = Movement->GetWalkSpeed();
	CachedBrakingFriction = Movement->BrakingFriction;
	CachedGroundFriction = Movement->GroundFriction;
	CachedBrakingFrictionFactor = Movement->BrakingFrictionFactor;
	CachedBrakingDecelerationWalking = Movement->BrakingDecelerationWalking;

	CachedMovementMode = Movement->MovementMode;
	bCachedIsMovingOnGround = Movement->IsMovingOnGround();
	bCachedIsCrouching = Movement->IsCrouching();
	bCachedOrientRotationToMovement = Movement->bOrientRotationToMovement;
	bCachedUseSeparateBrakingFriction = Movement->bUseSeparateBrakingFriction;
	InputVector = Movement->GetLastInputVector();

	// --- 3. GATHER CHARACTER STATE DATA ---
	LayerStates = Character->GetLayerStates();
	bIsLocallyControlled = Character->IsLocallyControlled();

	// --- 4. GATHER COMPLEX SOCKET TRANSFORMS ---
	GatherLeftHandTransform();
	GatherProceduralAimingTarget();

	// --- 5. TURN IN PLACE (Game Thread Execution) ---
	bIsStrafing = !bCachedOrientRotationToMovement;
	UTurnInPlaceStatics::UpdateTurnInPlace(
		Character->TurnInPlace,
		DeltaSeconds,
		TurnInPlaceAnimGraphData,
		bIsStrafing,
		TurnInPlaceAnimGraphOutput,
		bCanUpdateTurnInPlace
	);
}

// =========================================================================
//               PHASE 2: WORKER THREAD (Run Math in Parallel)
// =========================================================================
void UFPSAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	// If these values aren't initialized, exit safely.
	if (CachedMaxWalkSpeed == 0.0f && CachedGravityZ == 0.0f) return;

	// Run all complex math using CACHED data. No Actor queries allowed!
	UpdateLocationData(DeltaSeconds);
	UpdateRotationData(DeltaSeconds);
	UpdateVelocityData(DeltaSeconds);
	UpdateAccelerationData();
	UpdateCharacterState();
	UpdateAimingData(DeltaSeconds);
	UpdateJumpFallData();
	UpdateWallDetectionHeuristic();

	// Smooth the procedural aiming data we calculated on the Game Thread
	UpdateProceduralAimingInterp(DeltaSeconds);

	// Complete Turn in Place
	TurnInPlaceCurveValues = UTurnInPlaceStatics::ThreadSafeUpdateTurnInPlaceCurveValues(this, TurnInPlaceAnimGraphData);
	UTurnInPlaceStatics::ThreadSafeUpdateTurnInPlace(
		TurnInPlaceAnimGraphData, 
		bCanUpdateTurnInPlace, 
		bIsStrafing,
		TurnInPlaceAnimGraphOutput
	);
}

FTurnInPlaceAnimSet UFPSAnimInstance::GetTurnInPlaceAnimSet_Implementation() const
{
	if (bIsCrouching)
	{
		return TurnInPlaceAnimSetCrouched;
	} 
	return TurnInPlaceAnimSet;
}

#pragma endregion Lifecycle


#pragma region Math & Physics (Worker Thread)

void UFPSAnimInstance::UpdateLocationData(float DeltaTime)
{
	DisplacementSinceLastUpdate = UKismetMathLibrary::VSizeXY(CachedActorLocation - WorldLocation);
	WorldLocation = CachedActorLocation;

	DisplacementSpeed = UKismetMathLibrary::SafeDivide(DisplacementSinceLastUpdate, DeltaTime);

	Speed3D = WorldVelocity.Size();
	GroundSpeed = WorldVelocity2D.Size();
}

void UFPSAnimInstance::UpdateRotationData(float DeltaTime)
{
	if (bIsLocallyControlled)
	{
		// Local player: smooth both for visual quality
		SmoothedControlRotation = FMath::RInterpTo(
		   SmoothedControlRotation,
		   CachedControlRotation,
		   DeltaTime,
		   15.f
		);

		SmoothedActorRotation = FMath::RInterpTo(
		   SmoothedActorRotation,
		   CachedActorRotation,
		   DeltaTime,
		   15.f
		);
	}
	else
	{
		// Simulated proxy: Smooth both to keep the Delta stable
		SmoothedActorRotation = FMath::RInterpTo(SmoothedActorRotation, CachedActorRotation, DeltaTime, 15.f);
		
		// FIX: We MUST smooth the Control Rotation for proxies!
		// ReplicatedControlRotation arrives in discrete network packets. Interpolating it here 
		// stops the violent spine jitter while keeping the aim highly accurate.
		SmoothedControlRotation = FMath::RInterpTo(
			SmoothedControlRotation, 
			CachedControlRotation, 
			DeltaTime, 
			15.f // You can tweak this interpolation speed if the aim feels too "floaty" on proxies
		);
	}

	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(
	   SmoothedControlRotation,
	   SmoothedActorRotation
	);

	AimPitch = Delta.Pitch;
    
	// FIX: Both local and simulated proxies now use the exact Yaw difference.
	// This ensures the gun barrel points exactly where the ControlRotation dictates,
	// regardless of where the capsule is currently facing.
	RootYawOffset = Delta.Yaw;
}

void UFPSAnimInstance::UpdateVelocityData(float DeltaTime)
{
	bool bWasMovingLastUpdate = !LocalVelocity2D.IsNearlyZero(0.000001f);

	WorldVelocity = CachedVelocity;
	WorldVelocity2D = FVector(WorldVelocity.X, WorldVelocity.Y, 0.f);

	LocalVelocity2D = SmoothedActorRotation.UnrotateVector(WorldVelocity2D);
	bHasVelocity = !LocalVelocity2D.IsNearlyZero(0.000001f);

	if (WorldVelocity2D.SizeSquared() > 25.0f)
	{
		float VelocityYaw = WorldVelocity2D.ToOrientationRotator().Yaw;
		LocalVelocityDirectionAngle = FRotator::NormalizeAxis(VelocityYaw - CachedActorRotation.Yaw);
	}

	LocalVelocityDirectionAngleWithOffset = FRotator::NormalizeAxis(LocalVelocityDirectionAngle - RootYawOffset);

	FRotator CurrentRot(0.f, SmoothedLocomotionAngle, 0.f);
	FRotator TargetRot(0.f, LocalVelocityDirectionAngleWithOffset, 0.f);
	CurrentRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, LocomotionAngleInterpSpeed);
	SmoothedLocomotionAngle = CurrentRot.Yaw;

	LocalVelocityDirection = SelectCardinalDirectionFromAngle(
		LocalVelocityDirectionAngleWithOffset,
		CardinalDirectionDeadZone, LocalVelocityDirection,
		bWasMovingLastUpdate
	);
	
	LocalVelocityDirectionNoOffset = SelectCardinalDirectionFromAngle(
		LocalVelocityDirectionAngle,
		CardinalDirectionDeadZone,
		LocalVelocityDirectionNoOffset,
		bWasMovingLastUpdate
	);
}

void UFPSAnimInstance::UpdateAccelerationData()
{
	WorldAcceleration = CachedAcceleration;
	WorldAcceleration2D = FVector(WorldAcceleration.X, WorldAcceleration.Y, 0.f);
	LocalAcceleration2D = CachedActorRotation.UnrotateVector(WorldAcceleration2D);
	bHasAcceleration = !LocalAcceleration2D.IsNearlyZero(0.000001f);
}

void UFPSAnimInstance::UpdateCharacterState()
{
	bIsOnGround = bCachedIsMovingOnGround;

	bool WasCrouchingLastUpdate = bIsCrouching;
	bIsCrouching = bCachedIsCrouching;
	bCrouchStateChanged = bIsCrouching != WasCrouchingLastUpdate;

	bIsSprinting = (LayerStates.Gait == EGait::EG_Sprinting);
	bIsAiming = (LayerStates.AimState == EAimState::EAS_ADS || LayerStates.AimState == EAimState::EAS_Zoomed);

	bIsJumping = false;
	bIsFalling = false;
	if (CachedMovementMode == EMovementMode::MOVE_Falling)
	{
		if (WorldVelocity.Z > 0.f) bIsJumping = true;
		else bIsFalling = true;
	}

	bShouldMove = bHasAcceleration && (GroundSpeed > 3.0f);
}

void UFPSAnimInstance::UpdateAimingData(float DeltaTime)
{
	// Find the difference between where the capsule faces and where the camera aims
    FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(
       SmoothedControlRotation,
       SmoothedActorRotation
    );

    float HorizontalAngleOffset = 0.f;
    float VerticalAngleOffset = 0.f;

    // Only apply convergence math to proxies, and ensure Distance > 1.0f to avoid Divide-By-Zero crashes
    if (!bIsLocallyControlled && bIsArmed && CachedAimDistance > 1.0f)
    {
       // MATH EXPLANATION:
       // We use the Inverse Tangent (Atan2) to find the angle of a right triangle.
       // Opposite side = The physical offset of the gun from the camera (e.g., 25cm to the right)
       // Adjacent side = The distance to the target (e.g., 800cm forward)
       // Result = Returns Radians, which we convert to standard Unreal Degrees.
       HorizontalAngleOffset = FMath::RadiansToDegrees(FMath::Atan2(GunHorizontalOffsetCM, CachedAimDistance));
       VerticalAngleOffset = FMath::RadiansToDegrees(FMath::Atan2(GunVerticalOffsetCM, CachedAimDistance));
    }

    // --- STEP 1: BASE NETWORK INPUT + CALIBRATION ZEROING ---
    // We add our zeroing angle to perfectly level out the base animations at infinite distance.
    // We divide by 8.0f because this rotation will be applied individually across 8 spine/neck bones.
    float YawPerBone = (Delta.Yaw + GunYawZeroingAngle) / 8.0f;
    
    // We multiply Pitch by -1.0f because standard UE Mannequin skeletons bend backwards on positive pitch.
    float PitchPerBone = ((Delta.Pitch + GunPitchZeroingAngle) / 8.0f) * -1.0f;

    // --- STEP 2: APPLY DYNAMIC PARALLAX CONVERGENCE ---
    // Horizontal: Subtracting the calculated angle pulls the gun LEFT to compensate for a RIGHT shoulder.
    YawPerBone -= (HorizontalAngleOffset / 8.0f);
    
    // Vertical: Because GunVerticalOffsetCM is negative (-15), VerticalAngleOffset is negative.
    // Subtracting a negative number mathematically ADDS to the Pitch, bending the spine UP.
    PitchPerBone -= (VerticalAngleOffset / 8.0f); 

    // Combine into a target rotator (Roll is left at 0)
    FRotator TargetPitchPerBone(0.f, YawPerBone, PitchPerBone);

    // Smoothly interpolate the spine bending so network jitter doesn't snap the character's back
    RotationValuePerBone = FMath::RInterpTo(
       RotationValuePerBone,
       TargetPitchPerBone,
       DeltaTime,
       RotationPerBoneInterpSpeed
    );
	
	// ==========================================
	// DEEP DEBUG LOGGING (Throttled to 0.5s)
	// ==========================================
	if (!bIsLocallyControlled && bDrawProxyAimDebug)
	{
		static float LogTimer = 0.0f;
		LogTimer += DeltaTime;
        
		if (LogTimer >= 0.5f) // Print every half second
		{
			LogTimer = 0.0f;
            
			UE_LOG(LogTemp, Warning, TEXT("======================================"));
			UE_LOG(LogTemp, Warning, TEXT("PROXY AIM DATA | Distance: %.1f cm"), CachedAimDistance);
			UE_LOG(LogTemp, Log,     TEXT("1. Base Network Input  -> Pitch: %8.2f | Yaw: %8.2f"), Delta.Pitch, Delta.Yaw);
			UE_LOG(LogTemp, Log,     TEXT("2. Static Zeroing      -> Pitch: %8.2f | Yaw: %8.2f"), GunPitchZeroingAngle, GunYawZeroingAngle);
			UE_LOG(LogTemp, Log,     TEXT("3. Dynamic Convergence -> Vert : %8.2f | Horz: %8.2f"), VerticalAngleOffset, HorizontalAngleOffset);
			UE_LOG(LogTemp, Warning, TEXT("4. FINAL OUTPUT / BONE -> Pitch: %8.2f | Yaw: %8.2f"), PitchPerBone, YawPerBone);
			UE_LOG(LogTemp, Warning, TEXT("======================================"));
		}
	}
}

void UFPSAnimInstance::UpdateJumpFallData()
{
	if (bIsJumping && CachedGravityZ != 0.0f)
	{
		TimeToJumpApex = (0.0f - WorldVelocity.Z) / CachedGravityZ;
	}
	else
	{
		TimeToJumpApex = 0.0f;
	}
}

void UFPSAnimInstance::UpdateWallDetectionHeuristic()
{
	bool bLocalAccl = UKismetMathLibrary::VSizeXY(LocalAcceleration2D) > 0.1f;
	bool bLocalVel = UKismetMathLibrary::VSizeXY(LocalVelocity2D) < 200.0f;
	FVector LocalAcclNormalized = UKismetMathLibrary::Normal(LocalAcceleration2D);
	FVector LocalVelNormalized = UKismetMathLibrary::Normal(LocalVelocity2D);
	float DotProduct = FVector::DotProduct(LocalAcclNormalized, LocalVelNormalized);
	bIsRunningIntoWall = bLocalAccl && bLocalVel && UKismetMathLibrary::InRange_FloatFloat(DotProduct, -0.6f, 0.6f);
}

#pragma endregion Math & Physics

#pragma region Procedural Aiming

// Game Thread Function
void UFPSAnimInstance::GatherLeftHandTransform()
{
	AFPSPlayerCharacter* Character = Cast<AFPSPlayerCharacter>(TryGetPawnOwner());
	if (!Character || !Character->GetCombatComponent()) return;

	AFPSWeapon* EquippedWeapon = Character->GetCombatComponent()->GetEquippedWeapon();
	if (!EquippedWeapon || !EquippedWeapon->GetWeaponMesh()) return;

	FTransform SocketTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), RTS_World);

	FVector OutPosition;
	FRotator OutRotation;

	Character->GetMesh()->TransformToBoneSpace(
		HandBoneName,
		SocketTransform.GetLocation(),
		SocketTransform.Rotator(),
		OutPosition,
		OutRotation
	);

	LeftHandTargetTransform.SetLocation(OutPosition);
	LeftHandTargetTransform.SetRotation(FQuat(OutRotation));
}

/**
 * GAME THREAD EXECUTED:
 * Calculates the exact target transform for the right hand to ensure the currently selected
 * weapon sight perfectly aligns with the character's camera.
 */
void UFPSAnimInstance::GatherProceduralAimingTarget()
{
    AFPSPlayerCharacter* Character = Cast<AFPSPlayerCharacter>(TryGetPawnOwner());
    if (!Character) return;

    // 1. PROXY BAILOUT: Simulated proxies don't use this high-precision IK logic.
    if (!Character->IsLocallyControlled())
    {
       CurrentHandTransform = FTransform::Identity;
       HandLocation = FVector::ZeroVector;
       HandRotation = FRotator::ZeroRotator;
       return;
    }
    
    // 2. GATHER BASE TRANSFORMS
    // Hand: Where the right hand is currently.
    // Head: The pivot point we will move the hand relative to.
    FTransform HandWorldTransform = Character->GetMesh()->GetSocketTransform(HandBoneName, RTS_World);
    FTransform HeadWorldTransform = Character->GetMesh()->GetSocketTransform(HeadBoneName, RTS_World);
    
    // Camera: The absolute truth of where the player is looking.
    FTransform CameraWorldTransform;
    if (Character->IsLocallyControlled())
    {
       CameraWorldTransform = Character->GetCameraComponent()->GetComponentTransform();
    }
    else
    {
       // Reconstructed proxy camera (Fallback if the proxy bailout is removed later)
       FVector ProxyLoc = Character->GetMesh()->GetSocketLocation(HeadBoneName);
       FRotator ProxyRot = CachedControlRotation;
       CameraWorldTransform = FTransform(ProxyRot, ProxyLoc);
    }
    
    // 3. ITERATE OVER ALL SIGHTS ON THE WEAPON
    for (const FCachedSightData& Sight : CachedSights)
    {
       if (!Sight.Mesh.IsValid()) continue;

       FTransform FinalTransform = FTransform::Identity;

       // =====================================================================
       // SCENARIO A: OPTIC SIGHT (Red Dot / Holographic)
       // Easy math: We just snap the sight's single socket to the camera center.
       // =====================================================================
       if (Sight.bIsOptic)
       {
          FTransform SightTransform = Sight.Mesh->GetSocketTransform(Sight.SocketNameA, RTS_World);
          
          // Math: "If I hold the gun here, where is the sight relative to my hand?"
          FTransform HandToSight = UKismetMathLibrary::MakeRelativeTransform(HandWorldTransform, SightTransform);

          // Shift the weapon forward/backward along the X axis so it doesn't clip into our eyeball
          FVector AdjLoc = HandToSight.GetLocation();
          AdjLoc.X += CurrentDistanceFromCamera;

          FTransform AdjTrans(HandToSight.GetRotation(), AdjLoc, FVector::OneVector);
          
          // Math: "Apply this relative hand position to the absolute center of the camera."
          FTransform NewHandWorldTransform = AdjTrans * CameraWorldTransform;
          
          // Make it relative to the Head, since the AnimGraph expects Head-Space IK.
          FinalTransform = UKismetMathLibrary::MakeRelativeTransform(NewHandWorldTransform, HeadWorldTransform);
       }
       
       // =====================================================================
       // SCENARIO B: IRON SIGHTS (Front and Rear Alignment)
       // Hard math: We must mathematically bend the gun so the Front Post 
       // perfectly occludes the Rear Post from the camera's perspective.
       // =====================================================================
       else
       {
          FVector FrontLocation = Sight.Mesh->GetSocketLocation(Sight.SocketNameA);
          UMeshComponent* RearMeshToUse = Sight.RearMesh.IsValid() ? Sight.RearMesh.Get() : Sight.Mesh.Get();
          if (!RearMeshToUse) continue;

          FTransform RearTransform = Sight.Mesh->GetSocketTransform(Sight.SocketNameB, RTS_World);
          FVector RearLocation = RearTransform.GetLocation();
          FRotator RearRotation = RearTransform.Rotator();

          // 1. Find the 3D Vector pointing exactly from the Rear Sight to the Front Sight
          FVector DirectionToTarget = (FrontLocation - RearLocation).GetSafeNormal();
          
          // 2. Get the Rear Sight's local Up and Right axes
          FVector UpVector = UKismetMathLibrary::GetUpVector(RearRotation);
          FVector RightVector = UKismetMathLibrary::GetRightVector(RearRotation);

          // 3. DOT PRODUCTS: Project the Direction vector onto the Up and Right axes
          float UpDot = FVector::DotProduct(DirectionToTarget, UpVector);
          float RightDot = FVector::DotProduct(DirectionToTarget, RightVector);

          // 4. ARC SINES: Convert those dot products back into physical Pitch and Yaw angles
          float AnglePitch = UKismetMathLibrary::DegAsin(UpDot);
          float AngleYaw = UKismetMathLibrary::DegAsin(RightDot);

          // 5. Create rotators from these correction angles
          FRotator PitchRot = UKismetMathLibrary::RotatorFromAxisAndAngle(RightVector, -AnglePitch);
          FRotator YawRot = UKismetMathLibrary::RotatorFromAxisAndAngle(UpVector, AngleYaw);

          // 6. Compose the perfect alignment rotation
          FRotator FinalSightRotation = UKismetMathLibrary::ComposeRotators(
             UKismetMathLibrary::ComposeRotators(PitchRot, YawRot), RearRotation);

          // Create a new transform representing the PERFECTLY aligned rear sight
          FTransform TrueSightTransform(FinalSightRotation, RearLocation, FVector::OneVector);
          
          // Now apply the same logic as the Optic (Shift away from camera, convert to head-space)
          FTransform HandToSight = UKismetMathLibrary::MakeRelativeTransform(HandWorldTransform, TrueSightTransform);

          FVector AdjLoc = HandToSight.GetLocation();
          AdjLoc.X = CurrentDistanceFromCamera;

          FTransform AdjTrans(HandToSight.GetRotation(), AdjLoc, FVector::OneVector);
          FTransform NewHandWorld = AdjTrans * CameraWorldTransform;
          FinalTransform = UKismetMathLibrary::MakeRelativeTransform(NewHandWorld, HeadWorldTransform);
       }

       // Save to array based on the Sight Index (e.g., Index 0 = Primary, Index 1 = Canted)
       if (HandTransforms.IsValidIndex(Sight.SightIndex))
       {
          HandTransforms[Sight.SightIndex] = FinalTransform;
       }
    }

    // 4. CHOOSE THE FINAL TARGET BASED ON STATE (Hip vs ADS)
    if (bIsAiming)
    {
       if (HandTransforms.IsValidIndex(CurrentSightIndex))
          DesiredHandTransformTarget = HandTransforms[CurrentSightIndex];
    }
    else
    {
       FTransform HipTargetWorldTransform = CurrentHipFireOffset * CameraWorldTransform;
       DesiredHandTransformTarget = UKismetMathLibrary::MakeRelativeTransform(
          HipTargetWorldTransform, HeadWorldTransform);
    }
}

// Worker Thread Function
void UFPSAnimInstance::UpdateProceduralAimingInterp(float DeltaTime)
{
	if (!bIsLocallyControlled)
	{
		// Only stabilize, do not reposition.
		CurrentHandTransform = FTransform::Identity;
		HandLocation = FVector::ZeroVector;
		HandRotation = FRotator::ZeroRotator;
		return;
	}

	CurrentHandTransform = UKismetMathLibrary::TInterpTo(
		CurrentHandTransform,
		DesiredHandTransformTarget,
		DeltaTime,
		AimInterpSpeed
	);

	HandLocation = CurrentHandTransform.GetLocation();
	HandRotation = CurrentHandTransform.Rotator();
}

#pragma endregion Procedural Aiming

#pragma region Events & Utilities

ELocomotionCardinalDirection UFPSAnimInstance::SelectCardinalDirectionFromAngle(
	float NewAngle, float NewDeadZone, ELocomotionCardinalDirection CurrentDirection, bool bUseCurrentDirection) const
{
	float AbsoluteNewAngle = FMath::Abs(NewAngle);
	float ForwardDeadZone = NewDeadZone;
	float BackwardDeadZone = NewDeadZone;

	if (bUseCurrentDirection)
	{
		switch (CurrentDirection)
		{
		case ELocomotionCardinalDirection::LSD_Forward:
			ForwardDeadZone *= 2.0f;
			break;
		case ELocomotionCardinalDirection::LSD_Backward:
			BackwardDeadZone *= 2.0f;
			break;
		default:
			break;
		}
	}

	if (AbsoluteNewAngle <= (45.f + ForwardDeadZone)) return ELocomotionCardinalDirection::LSD_Forward;
	if (AbsoluteNewAngle >= (135.f - BackwardDeadZone)) return ELocomotionCardinalDirection::LSD_Backward;
	if (NewAngle > 0.0f) return ELocomotionCardinalDirection::LSD_Right;

	return ELocomotionCardinalDirection::LSD_Left;
}

void UFPSAnimInstance::OnCharacterWeaponEquipped(AFPSWeapon* NewWeapon)
{
	if (!NewWeapon)
	{
		CurrentHipFireOffset = FTransform::Identity;
		HandTransforms.Empty();
		CachedSights.Empty();
		
		GunHorizontalOffsetCM = 0.0f;
		GunVerticalOffsetCM = 0.0f;
		GunPitchZeroingAngle = 0.0f;
		GunYawZeroingAngle = 0.0f;
		return;
	}
	
	bIsArmed = IsValid(NewWeapon);
	CurrentHipFireOffset = NewWeapon->GetHipFireOffset();
	CurrentDistanceFromCamera = NewWeapon->GetDistanceFromCamera();
	CurrentRightHandJointTargetLocation = NewWeapon->GetRightHandJointTargetLocation();
	TimeToAim = NewWeapon->GetTimeToAim();
	TimeFromAim = NewWeapon->GetTimeFromAim();
	GunHorizontalOffsetCM = NewWeapon->GetGunHorizontalOffsetCM();
	GunVerticalOffsetCM = NewWeapon->GetGunVerticalOffsetCM();
	GunPitchZeroingAngle = NewWeapon->GetGunPitchZeroingAngle();
	GunYawZeroingAngle = NewWeapon->GetGunYawZeroingAngle();
	
	// 2. Build Sight Cache (Optimized Tag Parsing)
	CachedSights.Empty();
	TArray<UMeshComponent*> WeaponMeshes;
	NewWeapon->GetComponents<UMeshComponent>(WeaponMeshes);
	const FName OpticSocket = NewWeapon->GetOpticSocketName();
	const FName FrontSocket = NewWeapon->GetFrontSightSocketName();
	const FName RearSocket = NewWeapon->GetRearSightSocketName();
	const FString OpticPrefix = NewWeapon->GetOpticTagPrefix();
	const FString FrontPrefix = NewWeapon->GetFrontSightTagPrefix();
	const FString RearPrefix = NewWeapon->GetRearSightTagPrefix();

	for (UMeshComponent* Mesh : WeaponMeshes)
	{
		if (!Mesh) continue;
		for (FName Tag : Mesh->ComponentTags)
		{
			FString TagString = Tag.ToString();
			FString SightType, IndexStr;
			
			if (TagString.Split(TEXT("_"), &SightType, &IndexStr, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
			{
				int32 Index = FCString::Atoi(*IndexStr);
				// TYPE A: Red Dot
				if (SightType.Equals(OpticPrefix, ESearchCase::IgnoreCase) && Mesh->DoesSocketExist(OpticSocket))
				{
					FCachedSightData NewData;
					NewData.bIsOptic = true;
					NewData.SightIndex = Index;
					NewData.Mesh = Mesh;
					NewData.SocketNameA = OpticSocket;
					CachedSights.Add(NewData);
				}
				// TYPE B: Iron Sights (Requires Rear Search)
				else if (SightType.Equals(FrontPrefix, ESearchCase::IgnoreCase) && Mesh->DoesSocketExist(FrontSocket))
				{
					FCachedSightData NewData;
					NewData.bIsOptic = false;
					NewData.SightIndex = Index;
					NewData.Mesh = Mesh;
					NewData.SocketNameA = FrontSocket;

					if (Mesh->DoesSocketExist(RearSocket))
					{
						NewData.SocketNameB = RearSocket; // Same mesh
					}
					else
					{
						// Hunt for separate rear sight mesh
						FString TargetRearTag = RearPrefix + TEXT("_") + IndexStr;

						for (UMeshComponent* PotentialRear : WeaponMeshes)
						{
							if (PotentialRear->ComponentHasTag(FName(*TargetRearTag)) && PotentialRear->DoesSocketExist(RearSocket))
							{
								NewData.SocketNameB = RearSocket;
								NewData.RearMesh = PotentialRear;
								break;
							}
						}
					}
					CachedSights.Add(NewData);
				}
			}
		}
	}
	
	CachedSights.Sort();
	
	// Resize Transforms array
	if (CachedSights.Num() > 0)
	{
		int32 MaxIndex = 0;
		for (const auto& Sight : CachedSights) MaxIndex = FMath::Max(MaxIndex, Sight.SightIndex);
		HandTransforms.SetNum(MaxIndex + 1);
	}
}


float UFPSAnimInstance::GetPredictedStopDistance() const
{
	return UAnimCharacterMovementLibrary::PredictGroundMovementStopLocation(
		CachedLastUpdateVelocity,
		bCachedUseSeparateBrakingFriction,
		CachedBrakingFriction,
		CachedGroundFriction,
		CachedBrakingFrictionFactor,
		CachedBrakingDecelerationWalking
	).Size();
}

bool UFPSAnimInstance::ShouldDistanceMatchStop() const
{
	return bHasVelocity && !bHasAcceleration;
}

#pragma endregion

#pragma region Notifies

void UFPSAnimInstance::AnimNotify_WeaponGrab()
{
	// The exact frame the hand touches the gun, this fires!
	if (AFPSPlayerCharacter* Character = Cast<AFPSPlayerCharacter>(TryGetPawnOwner()))
	{
		if (UFPSCombatComponent* Combat = Character->GetCombatComponent())
		{
			Combat->FinishWeaponEquip();
		}
	}
}

#pragma endregion