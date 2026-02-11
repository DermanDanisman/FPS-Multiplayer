// © 2026 Heathrow (Derman Can Danisman). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Enums/FPSCharacterTypes.h"
#include "FPSCombatComponent.generated.h"

class AFPSPlayerCharacter;
class AFPSWeapon;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponEquippedDelegate, AFPSWeapon*, NewWeapon);

/**
 * Handles all combat logic: Firing, Reloading, Ammo Management, and Weapon State.
 * Acts as the "Brain" for the equipped weapon.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), PrioritizeCategories="Combat")
class FPS_MULTIPLAYER_API UFPSCombatComponent : public UActorComponent
{
    GENERATED_BODY()
    
public:
    UFPSCombatComponent();
    friend AFPSPlayerCharacter;
    
    // --- DELEGATES ---
    UPROPERTY(BlueprintAssignable, Category = "Combat|Delegates")
    FOnWeaponEquippedDelegate OnWeaponEquippedDelegate;
    
    // --- STATE GETTERS ---
    UFUNCTION(BlueprintPure, Category = "Combat|State")
    FORCEINLINE AFPSWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
    
    UFUNCTION(BlueprintPure, Category = "Combat|State")
    FORCEINLINE ECombatState GetCombatState() const { return CombatState; }
    
    UFUNCTION(BlueprintPure, Category = "Combat|State")
    FORCEINLINE int32 GetCarriedAmmo() const { return CarriedAmmo; }
    
    // --- COMBAT ACTIONS (Input) ---
    
    /** 
     * Handles the full logic of equipping a new weapon.
     * Automatically routes to Server if called on Client.
     */
    UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
    void EquipWeapon(AFPSWeapon* WeaponToEquip);
    
    /** Starts the firing sequence (Automatic or Single). */
    UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
    void StartFire();
    
    /** Stops the firing sequence/timer. */
    UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
    void StopFire();
    
    /** 
     * Initiates the reload process. 
     * Checks ammo, state, and magazine capacity before calling Server.
     */
    UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
    void Reload();
    
    /** 
     * Called via AnimNotify when the reload animation completes.
     * Calculates final ammo math and resets state.
     * @warning Must only execute on Server.
     */
    UFUNCTION(BlueprintCallable, Category="Combat|Internal")
    void FinishReloading();

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
    
    // --- SERVER RPCs ---

    UFUNCTION(Server, Reliable)
    void Server_EquipWeapon(AFPSWeapon* WeaponToEquip);

    UFUNCTION(Server, Reliable)
    void Server_Reload();
    
    // --- REPLICATION NOTIFIES ---

    UFUNCTION()
    void OnRep_EquippedWeapon(AFPSWeapon* LastEquippedWeapon);
    
    UFUNCTION()
    void OnRep_CombatState(ECombatState PreviousCombatState);

    // --- MULTICASTS (Visuals) ---
    
    /** Plays reload montage on all clients. */
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_Reload();

private:
    
    // --- INTERNAL LOGIC ---

    /** The actual fire logic tick (Trace -> Ammo -> Weapon Fire). */
    void Fire();

    /** 
     * Raycasts from the Camera center to finding the impact point.
     * @param TraceHitResult - Output struct containing hit info.
     */
    void TraceUnderCrosshairs(FHitResult& TraceHitResult);

    // --- INTERNAL STATE ---

    FTimerHandle FireTimerHandle;
    bool bFireButtonPressed;
    
    UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
    TObjectPtr<AFPSWeapon> EquippedWeapon;
    
    UPROPERTY(ReplicatedUsing = OnRep_CombatState)
    ECombatState CombatState;
    
    /** Total ammo in "Backpack" (Reserve). */
    UPROPERTY(Replicated)
    int32 CarriedAmmo = 120;
};