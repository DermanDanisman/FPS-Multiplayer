// © 2026 Heathrow (Derman Can Danisman). All rights reserved.


#include "UI/WidgetController/FPSWidgetController.h"

void UFPSWidgetController::BeginDestroy()
{
	Super::BeginDestroy();
}

void UFPSWidgetController::SetWidgetControllerParams(const FUIWidgetControllerParams& InWidgetControllerParams)
{
	// DEPENDENCY INJECTION: Assign all gameplay system references at once
	// This should be the first call after constructing the controller, before any UI binding occurs.
	// 
	// PARAMETER VALIDATION:
	// While we don't enforce non-null here, derived classes should validate critical references
	// before subscribing to delegates or broadcasting values (use check() or ensure() as appropriate).
	PlayerController       = InWidgetControllerParams.PlayerController;
	PlayerState            = InWidgetControllerParams.PlayerState;
}

void UFPSWidgetController::BroadcastInitialValues()
{
	// BASE CLASS IMPLEMENTATION: Intentionally empty (no-op).
	//
	// DERIVED CONTROLLER RESPONSIBILITIES:
	// Derived controllers (e.g., UHUDWidgetController) should override this method to:
	// 1. Read current values from the Core Data
	// 2. Broadcast these values via BlueprintAssignable delegates so widgets receive initial state
	// 3. Ensure UI displays correct values immediately after controller/widget setup
	//
	// INITIALIZATION ORDER:
	// This should be called AFTER:
	// - SetWidgetControllerParams() (establishes valid references)
	// - Widget.SetWidgetController() (widget is ready to receive broadcasts)
	// - BindCallbacksToDependencies() (ongoing updates are subscribed)
}

void UFPSWidgetController::BindCallbacksToDependencies()
{
	// BASE CLASS IMPLEMENTATION: Intentionally empty (no-op).
	//
	// DERIVED CONTROLLER RESPONSIBILITIES:
	// Derived controllers should override this method to:
	// 1. Subscribe to Core Data delegates for value changes
	// 2. Forward those changes via controller delegates for widgets to consume
	// 3. Subscribe to custom delegates
	//
	// BINDING PATTERN:
	// - Use AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate() for attribute changes
	// - Use AddLambda, AddUObject, or AddDynamic depending on lifetime management needs
	// - Forward received data through controller's BlueprintAssignable delegates
	//
	// INITIALIZATION ORDER:
	// This should be called AFTER:
	// - SetWidgetControllerParams()
	// - Widget.SetWidgetController() (widget delegates are ready to be bound to)
	// But BEFORE:
	// - BroadcastInitialValues() (ensures ongoing updates are wired before initial state push)
	//
	// EXAMPLE IMPLEMENTATION (in derived class):
	//   const UCoreAttributeSet* CoreAS = CastChecked<UCoreAttributeSet>(AttributeSet);
	//   
	//   AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(CoreAS->GetHealthAttribute())
	//     .AddLambda([this](const FOnAttributeChangeData& Data)
	//     {
	//         OnHealthChanged.Broadcast(Data.NewValue);
	//     });
	//
	//   // Subscribe to custom ASC events if available:
	//   if (UCoreAbilitySystemComponent* CoreASC = Cast<UCoreAbilitySystemComponent>(AbilitySystemComponent))
	//   {
	//       CoreASC->OnEffectAssetTags.AddLambda([this](const FGameplayTagContainer& Tags) { ... });
	//   }
}
