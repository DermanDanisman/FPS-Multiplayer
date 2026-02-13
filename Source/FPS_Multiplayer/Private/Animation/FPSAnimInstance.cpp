// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "FPS_Multiplayer/Public/Animation/FPSAnimInstance.h"

#include "Actor/Weapon/FPSWeapon.h"
#include "Camera/CameraComponent.h"
#include "Component/FPSCombatComponent.h"
#include "FPS_Multiplayer/Public/Character/FPSPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

// =========================================================================
//                           LIFECYCLE
// =========================================================================

void UFPSAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	UpdateReferences();
}

void UFPSAnimInstance::NativeUninitializeAnimation()
{
	if (FPSCharacter.IsValid())
	{
		if (UFPSCombatComponent* Combat = FPSCharacter->GetCombatComponent())
		{
			Combat->OnWeaponEquipped.Remove(OnWeaponEquippedDelegateHandle);
		}
	}
    
	Super::NativeUninitializeAnimation();
}

// Runs every frame on the Game Thread.
// ARCHITECTURE NOTE: We do NOT do heavy logic here. We simply copy data
// from the Character to local variables so the AnimGraph (Worker Thread) can read them safely.
void UFPSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	// 1. Safety Check: Ensure character exists before trying to access data.
	if (!FPSCharacter.IsValid())
	{
		UpdateReferences();
		if (!FPSCharacter.IsValid()) return;
	}

	// 2. Data Gathering (Cheap variable copying)
	CalculateEssentialData();
	CalculateLocomotionDirection();
	CalculateMovementDirectionMode();
	CalculateGaitValue();
	CalculatePlayRate();
	CalculatePitchValuePerBone();
	CalculateAimOffset();
	CalculateHandTransforms();
}

void UFPSAnimInstance::UpdateReferences()
{
	APawn* PawnOwner = TryGetPawnOwner();
	if (PawnOwner)
	{
		FPSCharacter = Cast<AFPSPlayerCharacter>(PawnOwner);
		if (FPSCharacter.IsValid())
		{
			// 1. SAFETY CHECK: Ensure Combat Component exists
			UFPSCombatComponent* Combat = FPSCharacter->GetCombatComponent();
			if (Combat)
			{
				// 2. SAFETY UNBIND: Prevent duplicate bindings or stale references
				// If we are re-initializing (e.g. changed weapon), remove the old bind first.
				Combat->OnWeaponEquipped.Remove(OnWeaponEquippedDelegateHandle);
                
				// 3. BIND
				OnWeaponEquippedDelegateHandle = Combat->OnWeaponEquipped.AddUObject(this, &ThisClass::OnCharacterWeaponEquipped);
			}

			MovementComponent = Cast<UCharacterMovementComponent>(FPSCharacter->GetMovementComponent());
            
			// ... Initialize your other cached variables here if needed ...
			if (Combat && Combat->GetEquippedWeapon())
			{
				// Optional: If we spawned late and weapon is ALREADY there, manually trigger the update
				// OnCharacterWeaponEquipped(Combat->GetEquippedWeapon());
			}
		}
	}
}

// =========================================================================
//                           CALCULATIONS
// =========================================================================

void UFPSAnimInstance::CalculateEssentialData()
{
	// -- Velocity & Speed --
	Velocity = MovementComponent.Get()->Velocity;
	GroundSpeed = FVector(Velocity.X, Velocity.Y, 0.f).Size(); // Ignore Z for locomotion

	// -- State Booleans --
	bIsAccelerating = (MovementComponent.Get()->GetCurrentAcceleration().SizeSquared() > 0.f);
	bIsFalling = MovementComponent.Get()->IsFalling();
	bIsCrouching = FPSCharacter->IsCrouched();
	bCanJump = FPSCharacter->CanJump();
    
	// "ShouldMove" gate: Only true if we have input AND reasonable speed.
	// Threshold (100.0f) prevents tiny micro-movements from triggering start animations.
	bShouldMove = bIsAccelerating && (GroundSpeed > 100.0f);
	
	// -- History Data (For future Stop/Pivot logic) --
	LastVelocityRotation = UKismetMathLibrary::MakeRotFromX(FPSCharacter->GetVelocity());
	bHasMovementInput = MovementComponent->GetLastInputVector().SizeSquared() > 0.f;
	LastMovementInputRotation = UKismetMathLibrary::MakeRotFromX(MovementComponent->GetLastInputVector());
	MovementInputVelocityDifference = UKismetMathLibrary::NormalizedDeltaRotator(LastMovementInputRotation, LastVelocityRotation).Yaw;
	
	// --- SYNC LAYER STATES ---
	// The AnimInstance simply copies the Authoritative State from the Character.
	// This ensures that if the Server says "Crouch", the Animation does it.
	LayerStates = FPSCharacter->GetLayerStates();
	bIsLocallyControlled = FPSCharacter->IsLocallyControlled();
	
	// [NEW] Cache Booleans for cleaner AnimGraph Transitions
	// It is much cheaper to check a boolean in the Graph than to switch on an Enum every frame
	bIsSprinting = (LayerStates.Gait == EGait::EG_Sprinting);
	bIsAiming    = (LayerStates.AimState == EAimState::EAS_ADS);
}

void UFPSAnimInstance::CalculateLocomotionDirection()
{
	// Don't update direction if standing still to prevent the value from snapping to 0.
	if (GroundSpeed < 100.0f || !FPSCharacter.IsValid()) return;

	// Calculate direction of movement relative to the character's facing direction.
	// Result: -180 to 180 degrees.
	FRotator ActorRotation = FPSCharacter.Get()->GetActorRotation();
	FRotator VelocityRotation = UKismetMathLibrary::MakeRotFromX(Velocity);
    
	Direction = UKismetMathLibrary::NormalizedDeltaRotator(VelocityRotation, ActorRotation).Yaw;
	LocomotionDirection = Direction;
}

void UFPSAnimInstance::CalculateMovementDirectionMode()
{
	// Hysteresis Logic: Prevents "Flickering" between Forward and Backward animations
	// when strafing exactly sideways (90 degrees).
	
	// Calculate Buffered Thresholds
	float WideMin = DirectionThresholdMin - Buffer; // e.g. -95
	float WideMax = DirectionThresholdMax + Buffer; // e.g.  95
	float NarrowMin = DirectionThresholdMin + Buffer; // e.g. -85
	float NarrowMax = DirectionThresholdMax - Buffer; // e.g.  85
    
	bool bInWideRange = UKismetMathLibrary::InRange_FloatFloat(Direction, WideMin, WideMax, true, true);
	bool bInNarrowRange = UKismetMathLibrary::InRange_FloatFloat(Direction, NarrowMin, NarrowMax, true, true);
    
	// Switch Logic
	if (!bInWideRange)
	{
		// Definitely moving backwards (e.g. 180 deg)
		MovementDirectionMode = EMovementDirectionMode::EMDM_Backward;
	}
	else if (bInNarrowRange)
	{
		// Definitely moving forwards (e.g. 0 deg)
		MovementDirectionMode = EMovementDirectionMode::EMDM_Forward;
	}
	// If in between (Grey Zone), keep the previous state.
}

void UFPSAnimInstance::CalculateGaitValue()
{
	// 1. Calculate the Raw Gait Value based on Speed
	// (This allows smooth blending as the character accelerates)
	float RawGait = 0.f;

	if (GroundSpeed > RunSpeed)
	{
		// Blending Run to Sprint (2.0 -> 3.0)
		RawGait = FMath::GetMappedRangeValueClamped(
			FVector2D(RunSpeed, SprintSpeed),
			FVector2D(RunGaitValue, SprintGaitValue), 
			GroundSpeed
		);
	}
	else if (GroundSpeed > WalkSpeed && GroundSpeed <= RunSpeed)
	{
		// Blending Walk to Run (1.0 -> 2.0)
		RawGait = FMath::GetMappedRangeValueClamped(
			FVector2D(WalkSpeed, RunSpeed), 
			FVector2D(WalkGaitValue, RunGaitValue), 
			GroundSpeed
		);
	}
	else
	{
		// Blending Stop to Walk (0.0 -> 1.0)
		RawGait = FMath::GetMappedRangeValueClamped(
			FVector2D(0.f, WalkSpeed), 
			FVector2D(0.0f, WalkGaitValue), 
			GroundSpeed
		);
	}
	
	// 2. STATE CLAMPING (The Polished Fix)
	// We limit the Gait Value based on the INTENT (The Enum).
	// This prevents "Floating Sprinting" if the character is pushed fast but isn't inputting sprint.
    
	float MaxAllowedGait = 3.0f; // Default to Sprint
    
	switch (LayerStates.Gait)
	{
	case EGait::EG_Walking:
		MaxAllowedGait = WalkGaitValue; // Cap at 1.0
		break;
	case EGait::EG_Running:
		MaxAllowedGait = RunGaitValue;  // Cap at 2.0
		break;
	case EGait::EG_Sprinting:
		MaxAllowedGait = SprintGaitValue; // Cap at 3.0
		break;
	default:
		MaxAllowedGait = RunGaitValue;
		break;
	}

	// We allow a small tolerance (0.1f) so blending doesn't snap instantly
	GaitValue = FMath::Min(RawGait, MaxAllowedGait + 0.1f);
}

void UFPSAnimInstance::CalculatePlayRate()
{
	// Stride Warping: Adjusts animation playback speed to prevent foot sliding.
	
	// 1. Safety Check
	if (GroundSpeed < 5.f || !FPSCharacter.IsValid())
	{
		PlayRate = 1.0f;
		return;
	}

	// 3. Calculate Individual Rates (The "Map Range Unclamped" Nodes)
	// If I am moving 300 units/sec, and Walk Anim is 150:
	// WalkRate = 300 / 150 = 2.0 (Play double speed)
	const float WalkRate = GroundSpeed / WalkSpeed;
	const float RunRate  = GroundSpeed / RunSpeed;
	const float SprintRate = GroundSpeed / SprintSpeed;

	// 4. Blend Rates based on Gait (The "Map Range Clamped" Node)
	// This matches the logic: "If Gait is 1.5, give me 50% WalkRate and 50% RunRate"
	float BlendedPlayRate = 1.0f;

	if (GaitValue <= 1.0f) // Walking
	{
		BlendedPlayRate = WalkRate;
	}
	else if (GaitValue <= 2.0f) // Blending Walk -> Run
	{
		BlendedPlayRate = FMath::GetMappedRangeValueClamped(
			FVector2D(1.f, 2.f),               // Input Range (Gait)
			FVector2D(WalkRate, RunRate),      // Output Range (Rates)
			GaitValue
		);
	}
	else // Blending Run -> Sprint
	{
		BlendedPlayRate = FMath::GetMappedRangeValueClamped(
			FVector2D(2.f, 3.f),               // Input Range (Gait)
			FVector2D(RunRate, SprintRate),    // Output Range (Rates)
			GaitValue
		);
	}

	// 5. Scale Adjustment (The "Divide" Node)
	const float ScaleZ = FPSCharacter->GetActorScale3D().Z;
	if (ScaleZ > 0.01f) // Avoid divide by zero
	{
		BlendedPlayRate /= ScaleZ;
	}

	// 6. Set Final Result
	PlayRate = BlendedPlayRate;
}

// Calculate up/down rotation for spine, neck and head bones when looking up/down
void UFPSAnimInstance::CalculatePitchValuePerBone()
{
	// 1. SOURCE SELECTION (Network Logic)
	FRotator TargetRotation;
	
	if (FPSCharacter->IsLocallyControlled())
	{
		// The control pitch already comes clamped to avoid looking more than 90 deg up or 90 deg down 
		// Local Input (Instant, Smooth)
		TargetRotation = FPSCharacter->GetControlRotation();
	}
	else
	{
		// Replicated Data (Networked)
		// This reads the compressed 'RemoteViewPitch' that Unreal replicates automatically.
		// Replicated, compressed byte (might be jittery)
		TargetRotation = FPSCharacter->GetBaseAimRotation();
	}
	
	// 2. NORMALIZE
	// GetBaseAimRotation often returns 0-360, while ControlRotation is -90 to 90.
	// NormalizeAxis forces everything into -180 to 180 range to prevent flipping bugs.
	FRotator NormalizedTarget = TargetRotation;
	NormalizedTarget.Pitch = FRotator::NormalizeAxis(TargetRotation.Pitch);
	
	// 3. APPLY YOUR MATH (The 180 flip)
	// We compose the rotation to get the "Look Up/Down" angle relative to the spine's forward.
	// The control pitch goes from 0 to 90 deg when looking up, and from 360 to 270 when looking down... 
	// So we rotate it by 180 deg to make it go from 0 to -90 (up) and from 0 to 90 deg (down), ready to be applied to the bones...
	FRotator AimRotation = UKismetMathLibrary::ComposeRotators(NormalizedTarget, FRotator(180.0f, 0.f, 0.f));
	
	// 4. DEFINE THE GOAL
	// We want the final bone rotation to be 1/8th of the total look angle.
	// Divide by 8 bones (head, neck_01, neck_02, spine_05, spine_04, spine_03, spine_02 and spine_01)
	FRotator TargetPitchPerBone = FRotator(0.f, 0.f, AimRotation.Pitch / 8.0f);
	
	// 5. INTERPOLATE (Smoothing)
    // Smoothly transition from Current Value -> Target Value
    PitchValuePerBone = FMath::RInterpTo(
        PitchValuePerBone,      // Current
        TargetPitchPerBone,     // Target
        GetDeltaSeconds(),      // Time since last frame (UAnimInstance provides this)
        PitchPerBoneInterpSpeed // Speed
    );
}

void UFPSAnimInstance::CalculateAimOffset()
{
	FRotator ControlRotation = FPSCharacter->GetControlRotation();
	FRotator ActorRotation = FPSCharacter->GetActorRotation();
	FRotator AimOffset = UKismetMathLibrary::NormalizedDeltaRotator(ControlRotation, ActorRotation);
	PitchOffset = AimOffset.Pitch;
}

// =========================================================================
//                        PROCEDURAL AIMING (The Math)
// =========================================================================

void UFPSAnimInstance::CalculateHandTransforms()
{
    // =========================================================
    // 1. SAFETY & NETWORK OPTIMIZATION
    // =========================================================
    
    // Safety: Ensure character and weapon exist
    if (!FPSCharacter.IsValid()) return;
    
    // [OPTIMIZATION] IsLocallyControlled Check
    // We strictly ONLY run this expensive math for the player controlling this character.
    // We do NOT want to calculate gun alignment for other players (Simulated Proxies).
    // They just play standard animations.
    if (!FPSCharacter->IsLocallyControlled()) 
    {
        HandLocation = FVector::ZeroVector;
        HandRotation = FRotator::ZeroRotator;
        return;
    }

    // =========================================================
    // 2. PREPARE WORLD DATA (Must run every frame)
    // =========================================================
    
    // We capture these transforms every frame because the player is constantly moving.
    // We use World Space as the "Common Language" to compare Bones vs Components.
    FTransform HandWorldTransform = FPSCharacter->GetMesh()->GetSocketTransform(HandBoneName, RTS_World);
    FTransform CameraWorldTransform  = FPSCharacter->GetCameraComponent()->GetComponentTransform();
    FTransform HeadWorldTransform = FPSCharacter->GetMesh()->GetSocketTransform(HeadBoneName, RTS_World);

    // Note: We use the CACHED float 'CurrentDistanceFromCamera' here instead of calling a function.

    // =========================================================
    // 3. ITERATE CACHED SIGHTS (The Optimization)
    // =========================================================
    
    // [OPTIMIZATION] Instead of "GetComponents" and "Split String" loops,
    // we iterate this clean array we built in OnCharacterWeaponEquipped.
    // This removes all memory allocation from the Tick loop.
    
    for (const FCachedSightData& Sight : CachedSights)
    {
        // Skip if the mesh was destroyed (weapon dropped/destroyed)
        if (!Sight.Mesh.IsValid()) continue;

        FTransform FinalTransform = FTransform::Identity;

        // --- LOGIC A: OPTIC SIGHTS (Red Dots) ---
        if (Sight.bIsOptic)
        {
            // 1. Get current World Location of the Red Dot socket
            FTransform SightTransform = Sight.Mesh->GetSocketTransform(Sight.SocketNameA, RTS_World);
            
            // 2. Calculate the Delta: Hand -> Sight
            FTransform HandToSight = UKismetMathLibrary::MakeRelativeTransform(HandWorldTransform, SightTransform);

            // 3. Apply Camera Distance (Cached)
            FVector AdjLoc = HandToSight.GetLocation();
            AdjLoc.X += CurrentDistanceFromCamera; 
            
            // 4. Calculate where the Hand needs to be in World Space
            FTransform AdjTrans(HandToSight.GetRotation(), AdjLoc, FVector::OneVector);
            FTransform NewHandWorldTransform = AdjTrans * CameraWorldTransform;
            
            // 5. Convert World Space -> Head Space (For AnimGraph)
            FinalTransform = UKismetMathLibrary::MakeRelativeTransform(NewHandWorldTransform, HeadWorldTransform);
        }
        // --- LOGIC B: IRON SIGHTS (Vector Math) ---
        else
        {
            // Note: For Iron Sights, we generally expect the Rear Sight to be static relative to the Front.
            // If you need the complex "Rear Component" search logic here, you should cache the 
            // RearComponent pointer in the FCachedSightData struct too.
            
            // This is the simplified version assuming Front/Rear are retrievable via the cached mesh/sockets.
            FVector FrontLocation = Sight.Mesh->GetSocketLocation(Sight.SocketNameA); // Front
        	
        	// Get Rear Transform
        	// CHECK: Is the rear sight on a separate mesh, or the main mesh?
        	UMeshComponent* RearMeshToUse = Sight.RearMesh.IsValid() ? Sight.RearMesh.Get() : Sight.Mesh.Get();
            
        	if (!RearMeshToUse) continue; // Safety
        	
            FTransform RearTransform = Sight.Mesh->GetSocketTransform(Sight.SocketNameB, RTS_World); // Rear
            
            FVector RearLocation = RearTransform.GetLocation();
            FRotator RearRotation = RearTransform.Rotator();

            // Vector math to align Front post perfectly with Rear notch
            FVector DirectionToTarget = (FrontLocation - RearLocation).GetSafeNormal();
            FVector UpVector = UKismetMathLibrary::GetUpVector(RearRotation);
            FVector RightVector = UKismetMathLibrary::GetRightVector(RearRotation);

            float UpDot = FVector::DotProduct(DirectionToTarget, UpVector);
            float RightDot = FVector::DotProduct(DirectionToTarget, RightVector);
            
            float AnglePitch = UKismetMathLibrary::DegAsin(UpDot);
            float AngleYaw = UKismetMathLibrary::DegAsin(RightDot);

            FRotator PitchRot = UKismetMathLibrary::RotatorFromAxisAndAngle(RightVector, -AnglePitch); 
            FRotator YawRot = UKismetMathLibrary::RotatorFromAxisAndAngle(UpVector, AngleYaw);

            FRotator CombinedCorrection = UKismetMathLibrary::ComposeRotators(PitchRot, YawRot);
            FRotator FinalSightRotation = UKismetMathLibrary::ComposeRotators(CombinedCorrection, RearRotation);

            FTransform TrueSightTransform(FinalSightRotation, RearLocation, FVector::OneVector);
            FTransform HandToSight = UKismetMathLibrary::MakeRelativeTransform(HandWorldTransform, TrueSightTransform);

            FVector AdjLoc = HandToSight.GetLocation();
            AdjLoc.X = CurrentDistanceFromCamera; // Cached
            
            FTransform AdjTrans(HandToSight.GetRotation(), AdjLoc, FVector::OneVector);
            FTransform NewHandWorld = AdjTrans * CameraWorldTransform;
            FinalTransform = UKismetMathLibrary::MakeRelativeTransform(NewHandWorld, HeadWorldTransform);
        }

        // Store the result directly into the array at the correct index
        if (HandTransforms.IsValidIndex(Sight.SightIndex))
        {
            HandTransforms[Sight.SightIndex] = FinalTransform;
        }
    }

    // =========================================================
    // 4. DETERMINE THE TARGET (The Decision)
    // =========================================================
    
    FTransform DesiredTarget = FTransform::Identity;

    // A. ARE WE AIMING? (ADS)
    if (LayerStates.AimState == EAimState::EAS_ADS || LayerStates.AimState == EAimState::EAS_Zoomed)
    {
        if (HandTransforms.IsValidIndex(CurrentSightIndex))
        {
            // Set target to the calculated ADS position
            DesiredTarget = HandTransforms[CurrentSightIndex];
        }
    }
    // B. ARE WE HIP FIRING? (Resting)
    else
    {
        // [OPTIMIZATION] Use the CACHED HipFireOffset.
        // We do not call Weapon->GetHipFireOffset() here anymore.
        
        // 1. Calculate World Position relative to Camera
        FTransform HipTargetWorldTransform = CurrentHipFireOffset * CameraWorldTransform;

        // 2. Convert to Head Space
        DesiredTarget = UKismetMathLibrary::MakeRelativeTransform(HipTargetWorldTransform, HeadWorldTransform);
    }

    // =========================================================
    // 5. INTERPOLATION (The Smoothing)
    // =========================================================
    // This MUST run every frame. TInterpTo moves 'Current' towards 'Target' 
    // by a tiny percentage based on DeltaTime.
	
    CurrentHandTransform = UKismetMathLibrary::TInterpTo(
       CurrentHandTransform, 
       DesiredTarget, 
       GetDeltaSeconds(), 
       AimInterpSpeed
    );

    // =========================================================
    // 6. OUTPUT TO ANIM GRAPH (The Final Result)
    // =========================================================
    
    HandLocation = CurrentHandTransform.GetLocation();
    HandRotation = CurrentHandTransform.Rotator();
}

void UFPSAnimInstance::OnCharacterWeaponEquipped(AFPSWeapon* NewWeapon)
{
	if (!NewWeapon) 
	{
		// Reset to defaults or safe values
		CurrentHipFireOffset = FTransform::Identity;
		HandTransforms.Empty();
		CachedSights.Empty();
		return; 
	}
	
	// 1. Cache Simple Data (The stuff you noticed!)
    const FWeaponMovementData& WeaponData = NewWeapon->GetMovementData();
    WalkSpeed   = WeaponData.AnimWalkRefSpeed;
    RunSpeed    = WeaponData.AnimRunRefSpeed;
    SprintSpeed = WeaponData.AnimSprintRefSpeed;

    CurrentHipFireOffset = NewWeapon->GetHipFireOffset();
    CurrentDistanceFromCamera = NewWeapon->GetDistanceFromCamera();
	CurrentRightHandEffectorLocation = NewWeapon->GetRightHandEffectorLocation();
	CurrentRightHandJointTargetLocation = NewWeapon->GetRightHandJointTargetLocation();
	CurrentLeftHandEffectorLocation = NewWeapon->GetLeftHandEffectorLocation();
	CurrentLeftHandJointTargetLocation = NewWeapon->GetLeftHandJointTargetLocation();
    TimeToAim = NewWeapon->GetTimeToAim();
    TimeFromAim = NewWeapon->GetTimeFromAim();

    // 2. BUILD THE SIGHT CACHE (The String Parsing Optimization)
    CachedSights.Empty();
    
    TArray<UMeshComponent*> WeaponMeshes;
    NewWeapon->GetComponents<UMeshComponent>(WeaponMeshes);

	const FName OpticSocket = NewWeapon->GetOpticSocketName();
	const FName FrontSocket = NewWeapon->GetFrontSightSocketName();
	const FName RearSocket  = NewWeapon->GetRearSightSocketName(); // "RearAimpoint"
    
	const FString OpticPrefix = NewWeapon->GetOpticTagPrefix();
	const FString FrontPrefix = NewWeapon->GetFrontSightTagPrefix();
	const FString RearPrefix  = NewWeapon->GetRearSightTagPrefix(); // "RearSight"

    // Heavy Loop: Runs ONCE per weapon equip.
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

            	// --- TYPE A: RED DOT / SCOPE ---
            	if (SightType.Equals(OpticPrefix, ESearchCase::IgnoreCase) && Mesh->DoesSocketExist(OpticSocket))
            	{
            		FCachedSightData NewData;
            		NewData.bIsOptic = true;
            		NewData.SightIndex = Index;
            		NewData.Mesh = Mesh;
            		NewData.SocketNameA = OpticSocket;
            		CachedSights.Add(NewData);
            	}
            	// --- TYPE B: IRON SIGHTS ---
            	// Entry Point: We MUST find the Front Sight tag first.
                else if (SightType.Equals(FrontPrefix, ESearchCase::IgnoreCase) && Mesh->DoesSocketExist(FrontSocket))
                {
                    // For Iron Sights, we need to find the rear sight component too. 
                    // (You can perform the Rear Sight Search logic here and store it in NewData.SocketNameB/Mesh)
                    // For brevity, I'm assuming you implement the Rear Search here similarly to your old code.
                    
                	FCachedSightData NewData;
                	NewData.bIsOptic = false;
                	NewData.SightIndex = Index;
                	NewData.Mesh = Mesh; // Front Sight is here
                	NewData.SocketNameA = FrontSocket;
                	// SMART CHECK: Is the Rear Sight on THIS SAME MESH?
                	if (Mesh->DoesSocketExist(RearSocket))
                	{
                		// YES! Standard Rifle. Both sockets on one mesh.
                		NewData.SocketNameB = RearSocket;
                		NewData.RearMesh = nullptr; // nullptr means "Use Main Mesh"
                	}
                	else
                	{
                		// NO! The rear sight must be a separate attachment.
                		// Let's hunt for it.
                		FString TargetRearTag = RearPrefix + TEXT("_") + IndexStr; // e.g., "RearSight_0"
                        
                		for (UMeshComponent* PotentialRear : WeaponMeshes)
                		{
                			if (PotentialRear->ComponentHasTag(FName(*TargetRearTag)) && PotentialRear->DoesSocketExist(RearSocket))
                			{
                				NewData.SocketNameB = RearSocket;
                				NewData.RearMesh = PotentialRear; // Found it!
                				break;
                			}
                		}
                	}
                    CachedSights.Add(NewData);
                }
            }
        }
    }

    // Sort sights so Index 0 is always first
    CachedSights.Sort();
    
    // Initialize HandTransforms array size
    if (CachedSights.Num() > 0)
    {
        // Find max index to size array correctly
        int32 MaxIndex = 0;
        for (const auto& Sight : CachedSights) MaxIndex = FMath::Max(MaxIndex, Sight.SightIndex);
        HandTransforms.SetNum(MaxIndex + 1);
    }
}
