// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FPSWeaponHandlerInterface.generated.h"

class AFPSWeapon;
// This class does not need to be modified.
UINTERFACE()
class UFPSWeaponHandlerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by Characters (Player or AI) that can handle weapons.
 */
class FPS_MULTIPLAYER_API IFPSWeaponHandlerInterface
{
	GENERATED_BODY()

public:
	/** 
	 * Generic command to equip a weapon.
	 * The Weapon calls this, and the Character decides HOW to do it (via CombatComponent).
	 */
	virtual void SetCurrentWeapon(AFPSWeapon* WeaponToEquip) = 0;
	
	/**
	 * Tells the character to play the firing animation.
	 * The Character decides if it should use the Hip or Aimed montage based on its own state.
	 */
	virtual void PlayFireMontage(UAnimMontage* HipFireMontage, UAnimMontage* AimedFireMontage) = 0;
	
	virtual void PlayEquipMontage(UAnimMontage* EquipMontage) = 0;
	
	virtual void PlayUnEquipMontage(UAnimMontage* UnEquipMontage) = 0;
};
