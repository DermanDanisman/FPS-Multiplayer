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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FString WeaponName = "Rifle";
	
	UPROPERTY(EditDefaultsOnly, Category = "Mesh")
	FName WeaponHandSocketName = "WeaponHandSocket";
	
	UPROPERTY(EditDefaultsOnly, Category = "Mesh")
	FName MuzzleSocketName = "Muzzle";
	
	// --- MOVEMENT DATA ---

	/** * Configuration for how this weapon affects player movement physics and animation.
	 * @usage 
	 * 1. Physics: Sets the CharacterMovementComponent's MaxWalkSpeed (e.g., Heavy weapons make you slow).
	 * 2. Animation: Sets the "Reference Speeds" for Stride Warping to prevent foot sliding.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	FWeaponMovementData WeaponMovementData;

	// --- COMBAT STATS ---
    
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FireDelay = 0.15f; // Fire Rate
    
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	bool bIsAutomatic = true;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float Range = 10000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float Damage = 20.f;
    
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	int32 MaxClipAmmo = 30;
	
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ballistics")
	EWeaponFireType FireType = EWeaponFireType::EWFT_HitScan;

	/** Only used if FireType == Projectile */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ballistics", meta=(EditCondition="FireType == EWeaponFireType::EWFT_Projectile"))
	TSubclassOf<AFPSProjectile> ProjectileClass;
	
	/** How much the gun kicks UP (Pitch). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Recoil")
	TObjectPtr<UCurveVector> RecoilPatternCurve;
	
	/** Multiplier to scale the curve values globally (e.g., 1.0 = Normal, 0.5 = Silenced) */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Recoil")
	float RecoilScale = 1.0f;
	
	/** How fast the recoil interpolates to the target. Higher = Snappier. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Recoil")
	float RecoilInterpSpeed = 30.0f;
    
	/** How fast the recoil recovers (centers back) when not firing. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Recoil")
	float RecoilRecoverySpeed = 20.0f;
    
	// --- ANIMATION & FX ---
	
	UPROPERTY(EditDefaultsOnly, Category = "Visuals|UI")
	TObjectPtr<UTexture2D> WeaponIcon; // The picture of the gun

	UPROPERTY(EditDefaultsOnly, Category = "Visuals|UI")
	TObjectPtr<UTexture2D> CrosshairTexture; // Optional: Shotgun vs Sniper crosshair
	
	UPROPERTY(EditDefaultsOnly, Category = "Visuals | Animation")
	TSubclassOf<UAnimInstance> EquippedAnimInstanceClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Visuals | Animation")
	TSubclassOf<UAnimInstance> UnEquippedAnimInstanceClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Visuals | Animation")
	TObjectPtr<UAnimMontage> EquipMontage;
	
	UPROPERTY(EditDefaultsOnly, Category = "Visuals | Animation")
	TObjectPtr<UAnimMontage> UnEquipMontage;
	
	/** * The animation to play on the Character when firing.
	 * @setup Create a Montage from your Fire Sequence. 
	 * @settings Inside the Montage, set the "Slot" to something like "ActionSlot" or "FireSlot".
	 * @note If your animation is Additive, ensure the Montage uses the correct Additive settings.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Visuals | Animation")
	TObjectPtr<UAnimMontage> FireMontage;
	
	UPROPERTY(EditDefaultsOnly, Category = "Visuals | Animation")
	TObjectPtr<UAnimMontage> AimedFireMontage;
	
	UPROPERTY(EditDefaultsOnly, Category = "Visuals | Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals | FX")
	TObjectPtr<UParticleSystem> MuzzleFlash;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals | FX")
	TObjectPtr<UParticleSystem> ImpactParticles;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals | Sound")
	TObjectPtr<USoundBase> FireSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Visuals | Sound")
	TObjectPtr<USoundBase> ReloadSound;
	
	/** * The Camera Shake class to play when firing.
	 * @usage Creates that "kick" feeling on the screen.
	 * @recommendation Use a 'MatineeCameraShake' or 'LegacyCameraShake' blueprint for easy tuning.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Visuals | Camera")
	TSubclassOf<UCameraShakeBase> FireCameraShake;

	// --- PROCEDURAL AIMING & IK CONFIGURATION ---

    /** * The forward offset (X-Axis) applied to the weapon when Aiming Down Sights (ADS).
     * @usage Lower values pull the sight closer to the eye. Higher values push it away.
     * @note Use this to prevent the rear sight from clipping into the camera or to adjust the "eye relief" of a scope.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Aiming | Position")
    float DistanceFromCamera = 0.f;

    /** * The procedural offset applied to the weapon when NOT aiming (Hip Fire).
     * @usage Controls the "resting" position of the gun on screen.
     * X = Forward/Back, Y = Right/Left, Z = Up/Down.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Aiming | Position")
    FTransform HipFireOffset;

    // --- TWO BONE IK (HAND PLACEMENT) ---
	
	/** * The procedural offset for the Right Hand IK Effector.
	 * @usage Generally stays at (0,0,0) as the right hand is the parent, but can be used to tweak grip alignment without re-importing meshes.
	 * @default (X=12.0, Y=3.0, Z=-5.0)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Aiming | Position | Two Bone IK")
	FVector RightHandEffectorLocation = FVector(12.f, 3.f, -5.f); 

	/** * The Pole Vector target for the Right Elbow.
	 * @usage Controls the direction the right elbow points. Critical for preventing the "Chicken Wing" look.
	 * @default (X=-68.0, Y=-40.0, Z=113.0)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Aiming | Position | Two Bone IK")
	FVector RightHandJointTargetLocation = FVector(-68.f, 40.f, 113.f);
	
	// --- ANIMATION TIMING (Visuals) ---

	/** * The duration (in Seconds) for the Animation Blueprint to blend from Hip Pose to ADS Pose.
	 * @usage Bind this to the "Blend Duration" in your AnimGraph State Machine (Hip -> ADS transition).
	 * @note Determines how fast the arms visually lift up. Does NOT affect the procedural math speed.
	 * @example Sniper: 0.4s (Slow lift) | Pistol: 0.1s (Fast snap)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Aiming | Timing", meta = (ClampMin = "0.01"))
	float TimeToAim = 0.25f;

	/** * The duration (in Seconds) for the Animation Blueprint to blend from ADS Pose back to Hip Pose.
	 * @usage Bind this to the "Blend Duration" in your AnimGraph State Machine (ADS -> Hip transition).
	 * @note Usually slightly faster than TimeToAim for responsiveness.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Aiming | Timing", meta = (ClampMin = "0.01"))
	float TimeFromAim = 0.2f;
	
	// --- SIGHT CONFIGURATION (Socket & Tag Linking) ---

    /** * The name of the socket on the Weapon Mesh that represents the "Red Dot" or "Crosshair" location.
     * @usage Procedural aiming will rotate the weapon until this socket is aligned with the Camera center.
     * @requirement Must exist on the mesh component tagged with 'OpticTagPrefix'.
     * @default "OpticAimpoint"
     */
    UPROPERTY(EditDefaultsOnly, Category = "Aiming | Sights")
    FName OpticSocketName = FName("OpticAimpoint");

    /** * The name of the socket placed at the TIP of the Front Iron Sight post.
     * @usage Used in combination with RearSightSocketName to calculate the alignment vector (Line of Sight).
     * @requirement Must exist on the mesh component tagged with 'FrontSightTagPrefix'.
     * @default "FrontAimpoint"
     */
    UPROPERTY(EditDefaultsOnly, Category = "Aiming | Sights")
    FName FrontSightSocketName = FName("FrontAimpoint");

    /** * The name of the socket placed at the CENTER of the Rear Iron Sight notch or peephole.
     * @usage The aim math pivots the gun around this point.
     * @requirement Must exist on the same mesh as the Front Sight, OR on a component tagged 'RearSightTagPrefix'.
     * @default "RearAimpoint"
     */
    UPROPERTY(EditDefaultsOnly, Category = "Aiming | Sights")
    FName RearSightSocketName = FName("RearAimpoint");

    /** * The Component Tag prefix used to identify Optic meshes (Red Dots, Scopes).
     * @usage The code looks for components with tags like "OpticSight_0", "OpticSight_1".
     * @note Do NOT include the "_0" index here. Just the prefix.
     * @default "OpticSight"
     */
    UPROPERTY(EditDefaultsOnly, Category = "Aiming | Sights")
    FString OpticTagPrefix = TEXT("OpticSight");

    /** * The Component Tag prefix used to identify the mesh containing the Front Iron Sight.
     * @usage Usually applied to the main weapon mesh tag array (e.g., "FrontSight_0").
     * @note This is the "Entry Point" for finding Iron Sights.
     * @default "FrontSight"
     */
    UPROPERTY(EditDefaultsOnly, Category = "Aiming | Sights")
    FString FrontSightTagPrefix = TEXT("FrontSight");

    /** * The Component Tag prefix used to identify the mesh containing the Rear Iron Sight.
     * @usage Only required if the Rear Sight is a separate attachment (e.g., a removable Carry Handle).
     * @note If the Rear Sight is on the main mesh (like an AK47), the code finds it automatically without this tag.
     * @default "RearSight"
     */
    UPROPERTY(EditDefaultsOnly, Category = "Aiming | Sights")
    FString RearSightTagPrefix = TEXT("RearSight");
};
