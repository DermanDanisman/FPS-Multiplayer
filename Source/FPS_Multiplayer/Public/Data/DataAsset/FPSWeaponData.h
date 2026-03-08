// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/Enums/FPSCharacterTypes.h"
#include "Engine/DataAsset.h"
#include "FPSWeaponData.generated.h"

// Forward Declarations
class AFPSProjectile;
class USoundBase;
class UParticleSystem;
class UAnimMontage;
class UCurveVector;

USTRUCT(BlueprintType)
struct FWeaponMovementData
{
	GENERATED_BODY()
	
	// Now you can choose Pistol, Rifle, Unarmed, etc. in the Data Asset!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Animation State")
	EOverlayState OverlayState = EOverlayState::EOS_Rifle;

	// --- PHYSICAL LIMITS (CharacterMovementComponent) ---
	// How fast the capsule is ALLOWED to move.
	
	UPROPERTY(EditAnywhere, Category = "Weapon|Movement Limits")
	float MaxBaseSpeed = 600.f; // Standard "Run" speed
	
	UPROPERTY(EditAnywhere, Category = "Weapon|Movement Limits")
	float MaxCrouchSpeed = 200.f;

	// --- ANIMATION REFERENCES (Stride Warping Math) ---
	// How fast the character moves in the ANIMATION file.
	
	UPROPERTY(EditAnywhere, Category = "Weapon|Animation Standards")
	float AnimWalkRefSpeed = 150.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Animation Standards")
	float AnimRunRefSpeed = 300.f;

	UPROPERTY(EditAnywhere, Category = "Weapon|Animation Standards")
	float AnimSprintRefSpeed = 600.f;
};

UENUM(BlueprintType)
enum EWeaponType
{
	// We will expand this with the child classes. Like for Melee: knife, sword etc. for Ranged: pistol, rifle etc.
	
	EWT_Melee  UMETA(DisplayName = "Melee"),
	EWR_Ranged UMETA(DisplayName = "Ranged")
};

UENUM(BlueprintType)
enum class EWeaponFireType : uint8
{
	EWFT_HitScan    UMETA(DisplayName = "Hit Scan"),    // Instant laser (CoD)
	EWFT_Projectile UMETA(DisplayName = "Projectile")   // Physics object (Battlefield/Halo)
};

/**
 * Defines the static configuration for a weapon archetype (e.g., "M4_Config", "Pistol_Config").
 * Designers edit this file, not the Actor Blueprint.
 */
UCLASS(BlueprintType)
class FPS_MULTIPLAYER_API UFPSWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, BlueprintReadOnly, Category = "Identity")
	FString WeaponName = "Rifle";
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMesh> WeaponMesh;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
	FName WeaponHandSocketName = "WeaponHandSocket";
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
	FName MuzzleSocketName = "Muzzle";
	
	// --- MOVEMENT DATA ---

	/** 
	 * Configuration for how this weapon affects player movement physics and animation.
	 * @usage 
	 * 1. Physics: Sets the CharacterMovementComponent's MaxWalkSpeed (e.g., Heavy weapons make you slow).
	 * 2. Animation: Sets the "Reference Speeds" for Stride Warping to prevent foot sliding.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	FWeaponMovementData WeaponMovementData;

	// --- COMBAT STATS ---
    
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float FireDelay = 0.15f; // Fire Rate
    
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	bool bIsAutomatic = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float Range = 10000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float Damage = 20.f;
    
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	int32 MaxClipAmmo = 30;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Ballistics")
	EWeaponFireType FireType = EWeaponFireType::EWFT_HitScan;

	/** Only used if FireType == Projectile */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Ballistics", meta=(EditCondition="FireType == EWeaponFireType::EWFT_Projectile"))
	TSubclassOf<AFPSProjectile> ProjectileClass;
	
	/** How much the gun kicks UP (Pitch). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Recoil")
	TObjectPtr<UCurveVector> RecoilPatternCurve;
	
	/** Multiplier to scale the curve values globally (e.g., 1.0 = Normal, 0.5 = Silenced) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Recoil")
	float RecoilScale = 1.0f;
	
	/** How fast the recoil interpolates to the target. Higher = Snappier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Recoil")
	float RecoilInterpSpeed = 30.0f;
    
	/** How fast the recoil recovers (centers back) when not firing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Recoil")
	float RecoilRecoverySpeed = 20.0f;
    
	// --- ANIMATION & FX ---
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals|UI")
	TObjectPtr<UTexture2D> WeaponIcon; // The picture of the gun

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals|UI")
	TObjectPtr<UTexture2D> CrosshairTexture; // Optional: Shotgun vs Sniper crosshair
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals | Animation")
	TSubclassOf<UAnimInstance> EquippedAnimInstanceClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals | Animation")
	TSubclassOf<UAnimInstance> UnEquippedAnimInstanceClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals | Animation")
	TObjectPtr<UAnimMontage> EquipMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals | Animation")
	TObjectPtr<UAnimMontage> UnEquipMontage;
	
	/** 
	 * The animation to play on the Character when firing.
	 * @setup Create a Montage from your Fire Sequence. 
	 * @settings Inside the Montage, set the "Slot" to something like "ActionSlot" or "FireSlot".
	 * @note If your animation is Additive, ensure the Montage uses the correct Additive settings.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals | Animation")
	TObjectPtr<UAnimMontage> FireMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals | Animation")
	TObjectPtr<UAnimMontage> AimedFireMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals | Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals | FX")
	TObjectPtr<UParticleSystem> MuzzleFlash;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals | FX")
	TObjectPtr<UParticleSystem> ImpactParticles;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals | Sound")
	TObjectPtr<USoundBase> FireSound;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals | Sound")
	TObjectPtr<USoundBase> ReloadSound;
	
	/** 
	 * The Camera Shake class to play when firing.
	 * @usage Creates that "kick" feeling on the screen.
	 * @recommendation Use a 'MatineeCameraShake' or 'LegacyCameraShake' blueprint for easy tuning.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals | Camera")
	TSubclassOf<UCameraShakeBase> FireCameraShake;
	
	// --- ANIMATION TIMING (Visuals) ---
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aiming")
	FTransform HipfireOffsetTransform;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aiming")
	FTransform ADSOffsetTransform;

	/**
	 *  The duration (in Seconds) for the Animation Blueprint to blend from Hip Pose to ADS Pose.
	 * @usage Bind this to the "Blend Duration" in your AnimGraph State Machine (Hip -> ADS transition).
	 * @note Determines how fast the arms visually lift up. Does NOT affect the procedural math speed.
	 * @example Sniper: 0.4s (Slow lift) | Pistol: 0.1s (Fast snap)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aiming | Timing", meta = (ClampMin = "0.01"))
	float TimeToAim = 0.25f;

	/**
	 * The duration (in Seconds) for the Animation Blueprint to blend from ADS Pose back to Hip Pose.
	 * @usage Bind this to the "Blend Duration" in your AnimGraph State Machine (ADS -> Hip transition).
	 * @note Usually slightly faster than TimeToAim for responsiveness.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aiming | Timing", meta = (ClampMin = "0.01"))
	float TimeFromAim = 0.2f;
};
