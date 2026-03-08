// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Data/Enums/FPSCharacterTypes.h"
#include "Data/Structs/FPSCharacterDataContainer.h"
#include "Interface/FPSAnimInterface.h"
#include "Kismet/KismetMathLibrary.h"
#include "FPSAnimInstanceBase.generated.h"

UENUM(BlueprintType)
enum class ERootYawOffsetMode : uint8
{
	BlendOut,
	Hold,
	Accumulate
};

/**
 * 
 */
UCLASS()
class FPS_MULTIPLAYER_API UFPSAnimInstanceBase : public UAnimInstance, public IFPSAnimInterface
{
	GENERATED_BODY()
	
public:
	
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUninitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
	//
	// --- IFPSAnimInterface Functions ---
	//
	virtual void SetFPSMode(bool bInFPSMode) override;
	virtual bool GetFPSMode() override { return bIsFPSMode; };
	virtual void SetUnarmed(bool bInUnarmed) override;
	virtual void SetADS(bool bInADS) override;
	virtual void SetADSUpper(bool bInADSUpper) override;
	virtual float GetCameraPitchWeight() override { return 0.0f; };
	virtual void SetFPSPelvisWeight(float InPelvisWeight) override;
	
	//
	// --- Public Helper Functions ---
	//
	/*UFUNCTION(BlueprintPure, Category = "Anim Instance Base", meta=(BlueprintThreadSafe))
	bool IsMovingPerpendicularToInitialPivot();*/
	
	UFUNCTION(meta=(BlueprintThreadSafe))
	FORCEINLINE void SetLastPivotTime(float NewPivotTime) { LastPivotTime = NewPivotTime; }
	
	//
	// --- Game Thread Gathering Data ---
	//
	
	UPROPERTY(Transient) FVector CachedActorLocation;
	UPROPERTY(Transient) FRotator CachedActorRotation;
	UPROPERTY(Transient) FVector CachedActorForwardVector;
	UPROPERTY(Transient) FVector CachedActorRightVector;
	UPROPERTY(Transient) FVector CachedActorUpVector;
	UPROPERTY(Transient) FVector CachedVelocity;
	UPROPERTY(Transient) FVector CachedAcceleration;
	UPROPERTY(Transient) FRotator CachedControlRotation;
	UPROPERTY(Transient) FVector CachedLastUpdateVelocity;

	UPROPERTY(Transient) float CachedGravityZ;
	UPROPERTY(Transient) float CachedJumpZVelocity;
	UPROPERTY(Transient) float CachedMaxWalkSpeed;
	UPROPERTY(Transient) float CachedCurrentWalkSpeed;
	UPROPERTY(Transient) float CachedBrakingFriction;
	UPROPERTY(Transient) float CachedGroundFriction;
	UPROPERTY(Transient) float CachedBrakingFrictionFactor;
	UPROPERTY(Transient) float CachedBrakingDecelerationWalking;

	UPROPERTY(Transient) TEnumAsByte<EMovementMode> CachedMovementMode;
	UPROPERTY(Transient) bool bCachedIsMovingOnGround;
	UPROPERTY(Transient) bool bCachedIsCrouching;
	UPROPERTY(Transient) bool bCachedOrientRotationToMovement;
	UPROPERTY(Transient) bool bCachedUseSeparateBrakingFriction;
    
	UPROPERTY(Transient) FTransform DesiredHandTransformTarget;
	UPROPERTY(Transient) FTransform HeadBoneTransform;
	
	//
	// --- Properties ---
	//
	
	UPROPERTY(BlueprintReadWrite, Category = "Properties")
	bool bIsFirstUpdate = true;
	
	UPROPERTY(BlueprintReadOnly, Category = "Properties")
	bool bEnableControlRig = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Properties")
	bool bUseFootPlacement = true;
	
	UPROPERTY(BlueprintReadWrite, Category = "Properties")
	bool bEnableRootYawOffset = true;
	
	UPROPERTY(BlueprintReadOnly, Category = "Properties")
	float GroundDistance;
	
	//
	// --- Rotation Data ---
	//
	
	UPROPERTY(BlueprintReadOnly, Category = "Rotation Data")
	FRotator WorldRotation;
	
	UPROPERTY(BlueprintReadOnly, Category = "Rotation Data")
	float YawDeltaSinceLastUpdate;
	
	UPROPERTY(BlueprintReadOnly, Category = "Rotation Data")
	float AdditiveLeanAngle;
	
	UPROPERTY(BlueprintReadOnly, Category = "Rotation Data")
	float YawDeltaSpeed;
	
	//
	// --- Location Data ---
	//
	
	UPROPERTY(BlueprintReadOnly, Category = "Location Data")
	FVector WorldLocation;
	
	UPROPERTY(BlueprintReadOnly, Category = "Location Data")
	float DisplacementSinceLastUpdate;
	
	UPROPERTY(BlueprintReadOnly, Category = "Location Data")
	float DisplacementSpeed;
	
	//
	// --- Velocity Data ---
	//
	
	UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
	FVector WorldVelocity;
	
	UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
	FVector LocalVelocity2D;
	
	UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
	float LocalVelocityDirectionAngle;
	
	UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
	float LocalVelocityDirectionAngleWithOffset;
	
	UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
	ELocomotionCardinalDirection LocalVelocityDirection;
	
	UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
	ELocomotionCardinalDirection LocalVelocityDirectionNoOffset;
	
	UPROPERTY(BlueprintReadOnly, Category = "Velocity Data")
	bool bHasVelocity;
	
	//
	// --- Acceleration Data ---
	//
	
	UPROPERTY(BlueprintReadOnly, Category = "Acceleration Data")
	FVector LocalAcceleration2D;
	
	UPROPERTY(BlueprintReadOnly, Category = "Acceleration Data")
	bool bHasAcceleration;
	
	UPROPERTY(BlueprintReadOnly, Category = "Acceleration Data")
	FVector PivotDirection2D;
	
	//
	// --- Character State Data ---
	//
	
	UPROPERTY(BlueprintReadOnly, Category = "Character State Data")
	bool bIsOnGround;

	UPROPERTY(BlueprintReadOnly, Category = "Character State Data")
	bool bIsCrouching;
	
	UPROPERTY(BlueprintReadOnly, Category = "Character State Data")
	bool bCrouchStateChange;
	
	UPROPERTY(BlueprintReadOnly, Category = "Character State Data")
	bool bADSStateChange;
	
	UPROPERTY(BlueprintReadOnly, Category = "Character State Data")
	bool bADSLastUpdate;
	
	UPROPERTY(BlueprintReadOnly, Category = "Character State Data")
	float TimeSinceFiredWeapon = 9999.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Character State Data")
	bool bIsJumping;
	
	UPROPERTY(BlueprintReadOnly, Category = "Character State Data")
	bool bIsFalling;
	
	UPROPERTY(BlueprintReadOnly, Category = "Character State Data")
	float TimeToJumpApex;
	
	UPROPERTY(BlueprintReadOnly, Category = "Character State Data")
	bool bIsRunningIntoWall;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
	bool bIsLocallyControlled;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Character State Data")
	FCharacterLayerStates LayerStates;
	
	
	//
	// --- Gameplay Tag Bindings Data ---
	//
	
	/**
	 * Bound to a gameplay tag from the Ability System Component. Search for "GameplayTagPropertyMap" in Class Defaults.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Gameplay Tag Bindings")
	bool bGameplayTagIsADS;
	
	UPROPERTY(BlueprintReadOnly, Category = "Gameplay Tag Bindings")
	bool bGameplayTagIsFiring;
	
	UPROPERTY(BlueprintReadOnly, Category = "Gameplay Tag Bindings")
	bool bGameplayTagIsReloading;
	
	UPROPERTY(BlueprintReadOnly, Category = "Gameplay Tag Bindings")
	bool bGameplayTagIsDashing;
	
	UPROPERTY(BlueprintReadOnly, Category = "Gameplay Tag Bindings")
	bool bGameplayTagIsMelee;
	
	//
	// --- Locomotion SM Data ---
	//
	
	UPROPERTY(BlueprintReadWrite, Category = "Locomotion SM Data")
	ELocomotionCardinalDirection StartDirection;
	
	UPROPERTY(BlueprintReadWrite, Category = "Locomotion SM Data")
	ELocomotionCardinalDirection PivotInitialDirection;
	
	UPROPERTY(BlueprintReadWrite, Category = "Locomotion SM Data")
	float LastPivotTime;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion SM Data")
	ELocomotionCardinalDirection CardinalDirectionFromAcceleration;
	
	// The new boolean for your AnimGraph Transition Rule
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion SM Data")
	bool bShouldCancelPivot;
	
	//
	// --- Blend Weight Data ---
	//
	
	UPROPERTY(BlueprintReadOnly, Category = "Blend Weight Data")
	float UpperbodyDynamicAdditiveWeight;
	
	//
	// --- Aiming Data ---
	//
	
	UPROPERTY(BlueprintReadWrite, Category = "Aiming")
	float AimPitch = -2.788732f;
	
	UPROPERTY(BlueprintReadWrite, Category = "Aiming")
	float AimYaw;
	
	UPROPERTY(BlueprintReadOnly, Category = "Aiming")
	FVector AimTargetLocation;
	
	//
	// --- Weapon Offset Data ---
	//
    
	UPROPERTY(BlueprintReadOnly, Category = "FPS | Weapon Offsets")
	FTransform HipfireOffsetTransform;
    
	UPROPERTY(BlueprintReadOnly, Category = "FPS | Weapon Offsets")
	FTransform ADSOffsetTransform;
    
	// This is the final mathematically blended transform sent to the AnimGraph!
	UPROPERTY(BlueprintReadOnly, Category = "FPS | Weapon Offsets")
	FTransform CurrentWeaponOffsetTransform;
	
	//
	// --- Settings Data ---
	//
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	float CardianalDirectionDeadZone = 10.f;
	
	// The tolerance threshold (0.15 seconds is usually perfect for 66hz tick rates)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	float PivotCancelTolerance = 0.15f;
	
	//
	// --- Linked Layered Data ---
	//
	
	UPROPERTY(BlueprintReadWrite, Category = "Linked Layer Data")
	bool bLinkedLayerChanged;
	
	UPROPERTY(BlueprintReadWrite, Category = "Linked Layer Data")
	TObjectPtr<UAnimInstance> LastLinkedLayerAnimInstance; 
	
	//
	// --- Turn In Place Data ---
	//
	
	UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
	float RootYawOffset;
	
	UPROPERTY(BlueprintReadOnly, Category = "Turn In Place")
	FFloatSpringState RootYawOffsetSpringState;
	
	UPROPERTY(BlueprintReadWrite, Category = "Turn In Place")
	float TurnYawCurveValue;
	
	UPROPERTY(BlueprintReadWrite, Category = "Turn In Place")
	ERootYawOffsetMode RootYawOffsetMode;

	/**
	 * Limit for how much we can offset the root's yaw during turn in place.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turn In Place")
	FVector2D RootYawOffsetAngleClamp = FVector2D(-120.f, 100.f);
	
	/**
	 * Limit for how much we can offset the root's yaw during turn in place.
	 * X = left limit
	 * Y = right limit
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Turn In Place")
	FVector2D RootYawOffsetAngleClampCrouched = FVector2D(-90.f, 80.f);
	
	
	//
	// --- FPS Data ---
	//
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS")
	float Pitch = -0.417859f;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS")
	FRotator PitchRotator;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS")
	bool bIsUnarmed;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS")
	bool bIsFPSMode;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS")
	bool bIsADSUpper;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS")
	bool bIsArmed;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS")
	float FPSPelvisWeight = 0.2f;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS")
	float ApplyPelvisWeight;
	
	//
	// --- FPS | Sway Data ---
	//
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS | Sway Data")
	FVector PitchOffsetPosition;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS | Sway Data")
	FVector CameraRotationOffset;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS | Sway Data")
	FRotator CurrentCameraRotation;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS | Sway Data")
	FRotator PreviousCameraRotation;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS | Sway Data")
	FRotator CameraRotationRate;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS | Sway Data")
	float SwayAlpha;
	
	//
	// --- FPS | Procedural Data ---
	//
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS | Procedural Data")
	FVector LocationLagPosition;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS | Procedural Data")
	float LocationLagAlpha;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS | Procedural Data")
	float RotationLagAlpha;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS | Procedural Data")
	FVector AirOffset;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS | Procedural Data")
	FRotator AirTiltRotator;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS | Procedural Data")
	float CrouchAlpha;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPS | Procedural Data")
	FTransform LeftHandTargetTransform;

	UFUNCTION(BlueprintCallable)
	bool ShouldEnableControlRig() const;
	
	void GatherLeftHandTransform();
	
	//
	// --- Turn In Place Functions ---
	//
	
	UFUNCTION(BlueprintCallable, Category = "Turn In Place", meta=(BlueprintThreadSafe))
	void SetRootYawOffset(float NewRootYawOffset);
	
	UFUNCTION(BlueprintCallable, Category = "Turn In Place", meta=(BlueprintThreadSafe))
	void ProcessTurnYawCurve();
	
private:
	
	// The hidden timer (Transient because we don't need to save this)
	UPROPERTY(Transient)
	float PivotCancelTimer;
	
	UPROPERTY(Transient) 
	float AimDistance = 20000.f;
	
	// Tracks the 0.0 to 1.0 transition between Hipfire and ADS
	UPROPERTY(Transient)
	float AimingAlpha = 0.0f;
	
	UPROPERTY(EditAnywhere, Category = "Aiming")
	bool bDrawProxyAimDebug = true;
	
	FDelegateHandle OnWeaponEquippedDelegateHandle;
	
	//
	// --- Thread Safe Functions ---
	//
	
	void UpdateLocationData(float DeltaTime);
	void UpdateRotationData(float DeltaTime);
	void UpdateVelocityData();
	void UpdateAccelerationData();
	void UpdateWallDetectionHeuristic();
	void UpdateCharacterStateData(float DeltaTime);
	void UpdateBlendWeightData(float DeltaTime);
	void UpdateRootYawOffset(float DeltaTime);
	void UpdateAimingData();
	void UpdateJumpFallData();
	void UpdatePivotState(float DeltaTime);
	void UpdateProceduralAlpha();
	void UpdateSwayData(float DeltaTime);
	void UpdateLagPosition(float DeltaTime);
	void UpdatePelvisWeight();
	
	//
	// --- Delegate Functions ---
	//
	
	UFUNCTION()
	void OnCharacterWeaponEquipped(AFPSWeapon* NewWeapon);
	
	//
	// --- Helper Functions ---
	//
	
	ELocomotionCardinalDirection SelectCardinalDirectionFromAngle(
		float NewAngle, 
		float NewDeadZone, 
		ELocomotionCardinalDirection CurrentDirection, 
		bool bUseCurrentDirection
	) const;
	
	ELocomotionCardinalDirection GetOppositeCardinalDirection(ELocomotionCardinalDirection CurrentDirection) const;
	
	//
	// --- Montage Notifies ---
	//
	UFUNCTION()
	void AnimNotify_WeaponGrab();
};
