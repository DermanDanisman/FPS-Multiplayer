// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPSCharacterTypes.generated.h"

// LAYER 1: The Physical Posture
UENUM(BlueprintType)
enum class EStance : uint8
{
	ES_Standing UMETA(DisplayName = "Standing"),
	ES_Crouching UMETA(DisplayName = "Crouching"),
	ES_Prone     UMETA(DisplayName = "Prone") // Future-proofing!
};

// LAYER 2: The Movement Speed (Gait)
UENUM(BlueprintType)
enum class EGait : uint8
{
	EG_Idle      UMETA(DisplayName = "Idle"),
	EG_Walking   UMETA(DisplayName = "Walking"),
	EG_Running   UMETA(DisplayName = "Running"),
	EG_Sprinting UMETA(DisplayName = "Sprinting")
};

// LAYER 3: The Active Action (Overrides everything)
UENUM(BlueprintType)
enum class EActionState : uint8
{
	EAS_None        UMETA(DisplayName = "None"),
	EAS_Sliding     UMETA(DisplayName = "Sliding"),
	EAS_Vaulting    UMETA(DisplayName = "Vaulting"),
	EAS_WallRunning UMETA(DisplayName = "Wall Running")
};

// LAYER 4: The "Overlay" (What are we holding?)
UENUM(BlueprintType)
enum class EOverlayState : uint8
{
	EOS_Unarmed     UMETA(DisplayName = "Unarmed"),
	EOS_Rifle       UMETA(DisplayName = "Rifle (2 Handed)"),
	EOS_Shotgun     UMETA(DisplayName = "Shotgun"),
	EOS_Pistol      UMETA(DisplayName = "Pistol (1 or 2 Handed)"),
	EOS_Knife       UMETA(DisplayName = "Knife (1 Handed)"),
};

// LAYER 5: The Aiming State (How are we targeting?)
UENUM(BlueprintType)
enum class EAimState : uint8
{
	EAS_None        UMETA(DisplayName = "Hip / None"),
	EAS_ADS         UMETA(DisplayName = "Aiming Down Sights"),
	EAS_PointAim    UMETA(DisplayName = "Point Aim"), // Future-proof
	EAS_Zoomed      UMETA(DisplayName = "Zoomed / Scoped")
};

// LAYER 6: The Combat Status (What is the gun doing?)
UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECS_Unoccupied      UMETA(DisplayName = "Unoccupied"), // Can Fire
	ECS_Firing          UMETA(DisplayName = "Firing"), // Currently Firing
	ECS_Reloading       UMETA(DisplayName = "Reloading"),  // Cannot Fire
	ECS_ThrowingGrenade UMETA(DisplayName = "Throwing Grenade"),
	ECS_SwappingWeapons UMETA(DisplayName = "Swapping Weapons")
};
