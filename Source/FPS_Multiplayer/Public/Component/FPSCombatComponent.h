// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Enums/FPSCharacterTypes.h"
#include "FPSCombatComponent.generated.h"

class AFPSPlayerCharacter;
class AFPSWeapon;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponEquippedDelegate, AFPSWeapon*, NewWeapon);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), PrioritizeCategories="CombatComponent")
class FPS_MULTIPLAYER_API UFPSCombatComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	// Sets default values for this component's properties
	UFPSCombatComponent();
	friend AFPSPlayerCharacter;
	
	UPROPERTY(BlueprintAssignable, Category = "CombatComponent|Delegates")
	FOnWeaponEquippedDelegate OnWeaponEquippedDelegate;
	
	UFUNCTION(BlueprintPure, Category = "CombatComponent|Getters")
	FORCEINLINE AFPSWeapon* GetEquippedWeapon() { return EquippedWeapon; }
	
	UFUNCTION(BlueprintPure, Category = "CombatComponent|Getters")
	FORCEINLINE ECombatState GetCombatState() const { return CombatState; }
	
	UFUNCTION(BlueprintPure, Category = "CombatComponent|Getters")
	FORCEINLINE int32 GetCarriedAmmo() const { return CarriedAmmo; }
	
	/**
	 * Main logic for equipping a weapon.
	 * Replication handled
	 */
	UFUNCTION(BlueprintCallable, Category = "CombatComponent")
	void EquipWeapon(AFPSWeapon* WeaponToEquip);
	
	UFUNCTION(BlueprintCallable, Category = "CombatComponent")
	void StartFire();
	
	UFUNCTION(BlueprintCallable, Category = "CombatComponent")
	void StopFire();
	
	UFUNCTION(BlueprintCallable, Category = "CombatComponent")
	void Reload();
	
	UFUNCTION(BlueprintCallable, Category="Combat")
	void FinishReloading();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	// --- RPCs (Remote Procedure Calls) ---

	/**
	 * The Bridge between Client Input and Server Logic.
	 * @param WeaponToEquip - The weapon the client wants to pick up.
	 * - Server: Means "Run this function on the Server".
	 * - Reliable: Means "This packet MUST arrive. Do not drop it." (Essential for gameplay actions).
	 */
	UFUNCTION(Server, Reliable)
	void Server_EquipWeapon(AFPSWeapon* WeaponToEquip);

	UFUNCTION()
	void OnRep_EquippedWeapon(AFPSWeapon* LastEquippedWeapon);
	
	UFUNCTION()
	void OnRep_CombatState(ECombatState PreviousCombatState);

private:
	
	// The Timer Function (Called repeatedly)
	void Fire();
	
	UFUNCTION(Server, Reliable)
	void Server_Reload();
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Reload();
	
	// The Eyes (Raycast)
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	// Timer Handle for Automatic Fire
	FTimerHandle FireTimerHandle;

	// Variable to track if button is held down
	bool bFireButtonPressed;
	
	/* * [GAMEPLAY STATE]
	 * Tracks the weapon currently in the player's hands.
	 * Replication: COND_None (Replicates to Everyone).
	 * Purpose: Everyone needs to see what gun I am holding.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	TObjectPtr<AFPSWeapon> EquippedWeapon;
	
	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState;
	
	UPROPERTY(Replicated)
	int32 CarriedAmmo = 120;
};
