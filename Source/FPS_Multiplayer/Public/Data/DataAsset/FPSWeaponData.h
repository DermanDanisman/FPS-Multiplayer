// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/Enums/FPSCharacterTypes.h"
#include "Engine/DataAsset.h"
#include "FPSWeaponData.generated.h"

// Forward Decls
class USoundBase;
class UParticleSystem;
class UAnimMontage;

USTRUCT(BlueprintType)
struct FWeaponMovementData
{
	GENERATED_BODY()
	
	EOverlayState OverlayState = EOverlayState::EOS_Rifle;

	// --- PHYSICAL LIMITS (CharacterMovementComponent) ---
	// How fast the capsule is ALLOWED to move.
	
	UPROPERTY(EditAnywhere, Category = "Weapon|Movement Limits")
	float MaxBaseSpeed = 600.f; // Standard "Run" speed
	
	UPROPERTY(EditAnywhere, Category = "Weapon|Movement Limits")
	float MaxCrouchSpeed = 300.f;

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

/**
 * Defines the static configuration for a weapon archetype (e.g., "M4_Config", "Pistol_Config").
 * Designers edit this file, not the Actor Blueprint.
 */
UCLASS(BlueprintType)
class FPS_MULTIPLAYER_API UFPSWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditDefaultsOnly, Category = "Identity")
	FString WeaponName = "Rifle";
	
	UPROPERTY(EditDefaultsOnly, Category = "Essential")
	FName WeaponHandSocketName = "WeaponHandSocketName";

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
    
	// --- ANIMATION & FX ---

	UPROPERTY(EditDefaultsOnly, Category = "Visuals | Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals | FX")
	TObjectPtr<UParticleSystem> MuzzleFlash;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals | FX")
	TObjectPtr<UParticleSystem> ImpactParticles;

	UPROPERTY(EditDefaultsOnly, Category = "Visuals | Sound")
	TObjectPtr<USoundBase> FireSound;

	// --- PROCEDURAL AIMING (Moved from Weapon.h) ---

	UPROPERTY(EditDefaultsOnly, Category = "Aiming | Position")
	FTransform HipFireOffset;
	
	UPROPERTY(EditDefaultsOnly, Category = "Aiming | Position")
	FVector LeftHandEffectorLocation;

	UPROPERTY(EditDefaultsOnly, Category = "Aiming | Position")
	float DistanceFromCamera = 0.f;

	/**
	 * The time (in Seconds) it takes to fully transition from Hip Fire to ADS.
	 * @usage Used to calculate the interpolation speed for procedural alignment.
	 * @warning Must be > 0.0f to avoid DivideByZero crashes in the AnimInstance.
	 * @recommendation Snipers: 0.4s | Rifles: 0.25s | Pistols: 0.15s
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Aiming | Timing", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float TimeToAim = 0.25f;

	/** 
	 * The time (in Seconds) it takes to return from ADS to Hip Fire.
	 * Usually faster than aiming in.
	 * @usage Used to calculate the interpolation speed for procedural alignment.
	 * @warning Must be > 0.0f to avoid DivideByZero crashes.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Aiming | Timing", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float TimeFromAim = 0.2f;
	
	// --- SIGHT CONFIGURATION ---
	UPROPERTY(EditDefaultsOnly, Category = "Aiming | Sights")
	FName OpticSocketName = FName("OpticAimpoint");

	UPROPERTY(EditDefaultsOnly, Category = "Aiming | Sights")
	FName FrontSightSocketName = FName("FrontAimpoint");

	UPROPERTY(EditDefaultsOnly, Category = "Aiming | Sights")
	FName RearSightSocketName = FName("RearAimpoint");

	UPROPERTY(EditDefaultsOnly, Category = "Aiming | Sights")
	FString OpticTagPrefix = TEXT("OpticSight");

	UPROPERTY(EditDefaultsOnly, Category = "Aiming | Sights")
	FString FrontSightTagPrefix = TEXT("FrontSight");

	UPROPERTY(EditDefaultsOnly, Category = "Aiming | Sights")
	FString RearSightTagPrefix = TEXT("RearSight");

	// --- MOVEMENT DATA ---
	// (You can move your FWeaponMovementData struct in here too!)
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	FWeaponMovementData WeaponMovementData;
};
