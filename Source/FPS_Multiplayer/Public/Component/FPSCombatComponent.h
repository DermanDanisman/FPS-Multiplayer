// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSCombatComponent.generated.h"

class AFPSPlayerCharacter;
class AFPSWeapon;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponEquippedDelegate, AFPSWeapon*, NewWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOverlappingWeaponChangeDelegate, AFPSWeapon*, OldWeapon, AFPSWeapon*, NewWeapon);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPS_MULTIPLAYER_API UFPSCombatComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	// Sets default values for this component's properties
	UFPSCombatComponent();
	friend AFPSPlayerCharacter;
	
	UPROPERTY(BlueprintAssignable, Category = "Combat Component | Delegates")
	FOnWeaponEquippedDelegate OnWeaponEquippedDelegate;
	
	UPROPERTY(BlueprintAssignable, Category = "Combat Component | Delegates")
	FOnOverlappingWeaponChangeDelegate OnOverlappingWeaponChange;
	
	// Setter for the Server to update the variable.
	// This function handles the visual update for the SERVER player (Host).
	UFUNCTION(BlueprintCallable, Category = "Combat Component | Setters")
	void SetOverlappingWeapon(AFPSWeapon* NewOverlappingWeapon);
	
	UFUNCTION(BlueprintPure, Category = "Combat Component | Getters")
	FORCEINLINE AFPSWeapon* GetOverlappingWeapon() { return OverlappingWeapon; }
	
	UFUNCTION(BlueprintPure, Category = "Combat Component | Getters")
	FORCEINLINE AFPSWeapon* GetEquippedWeapon() { return EquippedWeapon; }
	
	/**
	 * Main logic for equipping a weapon.
	 * Replication handled
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Component")
	void EquipWeapon(AFPSWeapon* WeaponToEquip);

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
	
	/*
	 * RepNotify for OverlappingWeapon.
	 * @param LastWeapon - Unreal automatically passes the OLD value of the variable before the update.
	 * We use LastWeapon to turn OFF the widget on the gun we just walked away from.
	 */
	UFUNCTION()
	void OnRep_OverlappingWeapon(AFPSWeapon* LastOverlappedWeapon);
	
private:
	
	// --- REPLICATED VARIABLES ---

	/* * [UI STATE]
	 * Tracks the weapon currently under the player's feet.
	 * Replication: OwnerOnly (Bandwidth optimization).
	 * Purpose: Only the local player needs to see the "Press E" prompt.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	TObjectPtr<AFPSWeapon> OverlappingWeapon;
	
	/* * [GAMEPLAY STATE]
	 * Tracks the weapon currently in the player's hands.
	 * Replication: COND_None (Replicates to Everyone).
	 * Purpose: Everyone needs to see what gun I am holding.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	TObjectPtr<AFPSWeapon> EquippedWeapon;
	
	UPROPERTY(EditAnywhere, Category = "Combat Component")
	FName CharacterWeaponSocket = "WeaponSocket";
};
