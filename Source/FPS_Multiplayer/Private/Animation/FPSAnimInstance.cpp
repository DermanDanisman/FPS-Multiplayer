// © 2025 Heathrow (Derman Can Danisman). All rights reserved.

#include "FPS_Multiplayer/Public/Animation/FPSAnimInstance.h"

#include "Actor/Weapon/FPSWeapon.h"
#include "Camera/CameraComponent.h"
#include "Component/FPSCombatComponent.h"
#include "FPS_Multiplayer/Public/Character/FPSPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"

// =========================================================================
//                           LIFECYCLE
// =========================================================================

void UFPSAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	UpdateReferences();
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
}

void UFPSAnimInstance::UpdateReferences()
{
	APawn* PawnOwner = TryGetPawnOwner();
	if (PawnOwner)
	{
		FPSCharacter = Cast<AFPSPlayerCharacter>(PawnOwner);
		if (FPSCharacter.IsValid())
		{
			FPSCharacter->GetCombatComponent()->OnWeaponEquippedDelegate.AddDynamic(this, &ThisClass::OnCharacterWeaponEquipped);
			MovementComponent = Cast<UCharacterMovementComponent>(FPSCharacter->GetMovementComponent());
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
	// Map current Speed to a Gait Value (0=Idle, 1=Walk, 2=Run, 3=Sprint)
	
	if (GroundSpeed > RunSpeed)
	{
		// Blending Run to Sprint (2.0 -> 3.0)
		GaitValue = FMath::GetMappedRangeValueClamped(
			FVector2D(RunSpeed, SprintSpeed),
			FVector2D(RunGaitValue, SprintGaitValue), 
			GroundSpeed
		);
	}
	else if (GroundSpeed > WalkSpeed && GroundSpeed <= RunSpeed)
	{
		// Blending Walk to Run (1.0 -> 2.0)
		GaitValue = FMath::GetMappedRangeValueClamped(
			FVector2D(WalkSpeed, RunSpeed), 
			FVector2D(WalkGaitValue, RunGaitValue), 
			GroundSpeed
		);
	}
	else
	{
		// Blending Stop to Walk (0.0 -> 1.0)
		GaitValue = FMath::GetMappedRangeValueClamped(
			FVector2D(0.f, WalkSpeed), 
			FVector2D(0.0f, WalkGaitValue), 
			GroundSpeed
		);
	}
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
	// --- 1. SAFETY & VALIDATION ---
	if (!FPSCharacter.IsValid()) return;
	UFPSCombatComponent* Combat = FPSCharacter->GetCombatComponent();
	if (!Combat) return;
	AFPSWeapon* Weapon = Combat->GetEquippedWeapon();
	if (!Weapon) return;
	
	// --- STABILITY FIX ---
	// We REMOVED "if (!IsLocallyControlled()) return" here.
	// Why? Because during the first frame of gameplay (Spawn), the Controller might not
	// possess the Pawn yet. If we return early, the array stays empty forever.
	// It is safer to run this calculation once on all machines than to risk missing it.
	
	// Caching aim times for the AnimGraph transition logic
	TimeToAim = Weapon->GetTimeToAim();
	TimeFromAim = Weapon->GetTimeFromAim();

    // --- 2. PREPARE DATA ---
    HandTransforms.Empty();
	// Memory Optimization: Pre-allocate 10 slots. 
	// This prevents the TArray from resizing/reallocating memory multiple times loop.
	//HandTransforms.SetNum(10);
    
    OpticSights.Empty();
    FrontSights.Empty();

	// Cache World Transforms once to avoid overhead in the loop
	// GetSocketTransform(RTS_World) performs a matrix multiplication, so caching it is good.
	FTransform HandWorld = FPSCharacter->GetMesh()->GetSocketTransform(HandBoneName, RTS_World);
	FTransform CameraWorld = FPSCharacter->GetCameraComponent()->GetComponentTransform();
	FTransform HeadWorld = FPSCharacter->GetMesh()->GetSocketTransform(HeadBoneName, RTS_World);
    
    // Weapon Configs
    float DistFromCam = Weapon->GetDistanceFromCamera();
    
	// Cache configuration strings from the Weapon Data Asset/Class
    const FName OpticSocket = Weapon->GetOpticSocketName();
    const FName FrontSocket = Weapon->GetFrontSightSocketName();
    const FName RearSocket  = Weapon->GetRearSightSocketName();
    
    const FString OpticPrefix = Weapon->GetOpticTagPrefix();
    const FString FrontPrefix = Weapon->GetFrontSightTagPrefix();
    const FString RearPrefix  = Weapon->GetRearSightTagPrefix();

	// --- 3. ITERATE WEAPON COMPONENTS ---
    TArray<UMeshComponent*> WeaponMeshes;
    Weapon->GetComponents<UMeshComponent>(WeaponMeshes);

    for (UMeshComponent* Mesh : WeaponMeshes)
    {
        if (!Mesh) continue;

    	// Loop through all tags on this specific mesh (e.g., "OpticSight_0", "FrontSight_1")
        for (FName Tag : Mesh->ComponentTags)
        {
            FString TagString = Tag.ToString();
            FString SightType, IndexStr;

        	// STRING PARSING: Split "OpticSight_0" into "OpticSight" and "0".
        	// Optimization: ESearchCase::IgnoreCase allows cleaner blueprinting without strict capitalization.
            if (TagString.Split(TEXT("_"), &SightType, &IndexStr, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
            {
            	// Convert "0" string to integer 0
                int32 SightIndex = FCString::Atoi(*IndexStr);
                
            	// Safety: If designer puts "OpticSight_99", expand array to prevent IndexOutOfBounds crash.
                if (HandTransforms.Num() <= SightIndex) HandTransforms.SetNum(SightIndex + 1);

            	// =========================================================
            	//                 LOGIC A: OPTIC SIGHTS (Red Dots)
            	// =========================================================
                // Compare against Variable, check for Variable Socket
                if (SightType.Equals(OpticPrefix, ESearchCase::IgnoreCase) && Mesh->DoesSocketExist(OpticSocket))
                {
                	// Store metadata for sorting/debugging later
                    FSightMeshData NewData;
                    NewData.Mesh = Mesh;
                    NewData.SightIndex = SightIndex;
                    OpticSights.Add(NewData);

                	// 1. Get where the Red Dot is currently in the world
                	FTransform SightTransform = Mesh->GetSocketTransform(OpticSocket, RTS_World);

                	// 2. Calculate the Delta: "How far is the Hand from the Sight?"
                	FTransform HandToSight = UKismetMathLibrary::MakeRelativeTransform(HandWorld, SightTransform);

                	// 3. Apply the Camera Offset (X-Axis)
                	// We keep the Rotation/YZ offset calculated above, but force X to our desired camera distance.
                	FVector AdjustedLoc = HandToSight.GetLocation();
                	AdjustedLoc.X += DistFromCam; // Pushes gun forward so it doesn't clip into face
                	FTransform AdjustedHandToSight(HandToSight.GetRotation(), AdjustedLoc, FVector::OneVector);

                	// 4. Calculate Final World Position
                	// "Put the hand relative to the Camera, using the offset we just calculated."
                	FTransform NewHandWorld = AdjustedHandToSight * CameraWorld;
                    
                	// 5. Convert to Head Space
                	// The AnimGraph usually modifies bones in Component or Parent Bone Space.
                	// Converting to Head Space makes the gun follow the head's movements (breathing/bobbing).
                	FTransform FinalTransform = UKismetMathLibrary::MakeRelativeTransform(NewHandWorld, HeadWorld);

                	HandTransforms[SightIndex] = FinalTransform;
                }
                
            	// =========================================================
            	//               LOGIC B: IRON SIGHTS (Vector Math)
            	// =========================================================
                else if (SightType.Equals(FrontPrefix, ESearchCase::IgnoreCase) && Mesh->DoesSocketExist(FrontSocket))
                {
                	// Store metadata for sorting/debugging later
                	FSightMeshData NewData;
                	NewData.Mesh = Mesh;
                	NewData.SightIndex = SightIndex;
                	FrontSights.Add(NewData);

                	// 1. Construct the Search Tag for the Rear Sight (e.g. "RearSight_0")
                	FString RearTag = RearPrefix + TEXT("_") + IndexStr;
                    
                	// 2. Find the Component that represents the Rear Sight
                	USceneComponent* RearSightComp = nullptr;
                	TArray<UActorComponent*> AllComps;
                	Weapon->GetComponents(AllComps);
                	
                    for (UActorComponent* Comp : AllComps)
                    {
                        if (Comp->ComponentHasTag(FName(*RearTag)))
                        {
                            RearSightComp = Cast<USceneComponent>(Comp);
                            break;
                        }
                    }

                    // Check for Variable Socket
                    if (RearSightComp && RearSightComp->DoesSocketExist(RearSocket))
                    {
                        // 3. Get Positions of Front Post and Rear Notch
                        FVector FrontLoc = Mesh->GetSocketLocation(FrontSocket);
                        FTransform RearTransform = RearSightComp->GetSocketTransform(RearSocket, RTS_World);
                        FVector RearLoc = RearTransform.GetLocation();
                        FRotator RearRot = RearTransform.Rotator();

                        // 4. ALIGNMENT MATH
                        // We need a vector pointing EXACTLY from Rear -> Front.
                        FVector DirectionToTarget = (FrontLoc - RearLoc).GetSafeNormal();
                        
                        FVector UpVector = UKismetMathLibrary::GetUpVector(RearRot);
                        FVector RightVector = UKismetMathLibrary::GetRightVector(RearRot);

                        // Dot Product checks alignment. 
                        // If Dot == 0, vectors are perpendicular. If Dot == 1, they are parallel.
                        float UpDot = FVector::DotProduct(DirectionToTarget, UpVector);
                        float RightDot = FVector::DotProduct(DirectionToTarget, RightVector);
                        
                        // Convert Dot Product (Cos/Sin relationship) to an Angle in Degrees
                        float AnglePitch = UKismetMathLibrary::DegAsin(UpDot);
                        float AngleYaw = UKismetMathLibrary::DegAsin(RightDot);

                        // Create Rotators to correct the misalignment
                        // Pitch rotates around Right Vector. Yaw rotates around Up Vector.
                        FRotator PitchRot = UKismetMathLibrary::RotatorFromAxisAndAngle(RightVector, -AnglePitch); 
                        FRotator YawRot = UKismetMathLibrary::RotatorFromAxisAndAngle(UpVector, AngleYaw);

                        // Combine corrections with the original Rear Rotation
                        FRotator CombinedCorrection = UKismetMathLibrary::ComposeRotators(PitchRot, YawRot);
                        FRotator FinalSightRot = UKismetMathLibrary::ComposeRotators(CombinedCorrection, RearRot);

                        // 5. Construct the "Perfect" Sight Transform
                        FTransform TrueSightTransform(FinalSightRot, RearLoc, FVector::OneVector);
                        
                        // 6. Calculate Hand Offset based on this "Perfect" transform
                        FTransform HandToSight = UKismetMathLibrary::MakeRelativeTransform(HandWorld, TrueSightTransform);

                        FVector AdjustedLoc = HandToSight.GetLocation();
                        AdjustedLoc.X = DistFromCam;
                        FTransform AdjustedHandToSight(HandToSight.GetRotation(), AdjustedLoc, FVector::OneVector);

                        FTransform NewHandWorld = AdjustedHandToSight * CameraWorld;
                        FTransform FinalTransform = UKismetMathLibrary::MakeRelativeTransform(NewHandWorld, HeadWorld);

                        HandTransforms[SightIndex] = FinalTransform;
                    }
                }
            }
        }
    }

	// --- 4. FINALIZE ---
	// If the currently selected sight is valid, use it. Otherwise, fallback to Iron Sights (Index 0).
	if (HandTransforms.IsValidIndex(CurrentSightIndex) && !HandTransforms[CurrentSightIndex].GetLocation().IsZero())
	{
		HandLocation = HandTransforms[CurrentSightIndex].GetLocation();
		HandRotation = HandTransforms[CurrentSightIndex].Rotator();
	}
	else if (HandTransforms.IsValidIndex(0) && !HandTransforms[0].GetLocation().IsZero())
	{
		HandLocation = HandTransforms[0].GetLocation();
		HandRotation = HandTransforms[0].Rotator();
		CurrentSightIndex = 0;
	}
}

void UFPSAnimInstance::OnCharacterWeaponEquipped(AFPSWeapon* NewWeapon)
{
	// 1. Safety Check
	if (!IsValid(NewWeapon))
	{
		// Optional: Reset to defaults if weapon is null
		return;
	}

	// 3. GET DATA
	// We strictly use this event to update MATH CONSTANTS (Stride Warping).
	//Update Stride Warping constants based on weapon weight/type
	const FWeaponMovementData& WeaponData = NewWeapon->GetMovementData();
	// 4. UPDATE ANIMATION CONFIGURATION
	WalkSpeed   = WeaponData.AnimWalkRefSpeed;
	RunSpeed    = WeaponData.AnimRunRefSpeed;
	SprintSpeed = WeaponData.AnimSprintRefSpeed;
		
	// 5. RUN MATH ONCE
	// This is the key optimization. We calculate the aiming offsets ONLY when the weapon changes.
	// We do NOT recalculate this every frame in NativeUpdateAnimation.
	CalculateHandTransforms();

	// CRITICAL: DO NOT set LayerStates.OverlayState here!
	// Why? Because NativeUpdateAnimation() runs every frame and copies the 
	// authoritative state from the Character. If you set it here, it will 
	// just get overwritten 1 frame later.
}
