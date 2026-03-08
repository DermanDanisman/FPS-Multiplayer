// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "UI/WidgetController/HUD/FPSOverlayController.h"

#include "Actor/Weapon/FPSWeapon.h"
#include "Character/FPSPlayerCharacter.h"
#include "Component/FPSCombatComponent.h"


void UFPSOverlayController::BeginDestroy()
{
	// Cleanup
	if (IsValid(GetCombatComponent()))
	{
		GetCombatComponent()->OnWeaponEquipped.RemoveAll(this);
	}

	Super::BeginDestroy();
}

void UFPSOverlayController::BroadcastInitialValues()
{
	UFPSCombatComponent* CombatComponent = GetCombatComponent();
	if (!CombatComponent) return;
	
	// 2. Pull Data & Push to View
	// Carried Ammo
	OnCarriedAmmoChanged.Broadcast(CombatComponent->GetCarriedAmmo());
	
	// Weapon Ammo (Only if we spawned with a gun)
	if (AFPSWeapon* Weapon = GetCombatComponent()->GetEquippedWeapon())
	{
		OnWeaponAmmoChanged.Broadcast(Weapon->GetCurrentClipAmmo(), Weapon->GetWeaponData()->MaxClipAmmo);
	}
}

void UFPSOverlayController::BindCallbacksToDependencies()
{
	// When the Controller is destroyed, the WeakLambda detects that this is stale and simply ignores the broadcast. It's safe.
	
	UFPSCombatComponent* CombatComponent = GetCombatComponent();
	if (CombatComponent)
	{
		// 1. Bind to Carried Ammo (Lambda)
		// We capture [this] so we can access our own member variable (OnCarriedAmmoChanged)
		CombatComponent->OnCarriedAmmoChanged.AddWeakLambda(this,[this](int32 NewAmmo)
		{
			OnCarriedAmmoChanged.Broadcast(NewAmmo);
		});

		// 2. Bind to Weapon Ammo (Lambda)
		CombatComponent->OnWeaponAmmoChanged.AddWeakLambda(this, [this](int32 Current, int32 Max)
		{
			OnWeaponAmmoChanged.Broadcast(Current, Max);
		});

		// 3. Bind to Weapon Equipped
		// When we switch guns, we need to refresh the numbers immediately!
		CombatComponent->OnWeaponEquipped.AddUObject(this, &UFPSOverlayController::OnWeaponEquipped);
	}
	
	if (AFPSPlayerCharacter* PlayerCharacter = Cast<AFPSPlayerCharacter>(PlayerController->GetPawn()))
	{
		PlayerCharacter->OnGaitStateChanged.AddWeakLambda(this, [this](EGait NewState)
		{
			OnGaitChangedUI.Broadcast(NewState);
		});
		
		PlayerCharacter->OnOverlayStateChanged.AddWeakLambda(this, [this](EOverlayState NewState)
		{
			OnOverlayStateChangedUI.Broadcast(NewState);
		});
		
		PlayerCharacter->OnAimStateChanged.AddWeakLambda(this, [this](EAimState NewState)
		{
			OnAimStateChangedUI.Broadcast(NewState);
		});
	}
}

void UFPSOverlayController::OnWeaponEquipped(AFPSWeapon* NewWeapon)
{
	if (!NewWeapon || !NewWeapon->GetWeaponData()) return;

	// 1. Update the "Static" Visuals (Happens once)
	if (NewWeapon->GetWeaponData()->WeaponIcon)
	{
		OnWeaponIconChanged.Broadcast(NewWeapon->GetWeaponData()->WeaponIcon);
		OnCrosshairChangedUI.Broadcast(NewWeapon->GetWeaponData()->CrosshairTexture);
	}

	// 2. Update the "Initial" Numbers (Happens once)
	OnWeaponAmmoChanged.Broadcast(NewWeapon->GetCurrentClipAmmo(), NewWeapon->GetWeaponData()->MaxClipAmmo);
	OnCarriedAmmoChanged.Broadcast(GetCombatComponent()->GetCarriedAmmo());
}

UFPSCombatComponent* UFPSOverlayController::GetCombatComponent() const
{
	if (!IsValid(PlayerController)) return nullptr;
	
	ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerController->GetPawn());
	if (IsValid(PlayerCharacter))
	{
		return PlayerCharacter->FindComponentByClass<UFPSCombatComponent>();
	}
	return nullptr;
}
