// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AnimTools : ModuleRules
{
	public AnimTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"PhysicsCore",              // Required for EPhysicalSurface
				// EDITOR LIBRARIES (Safe here!)
				"UnrealEd",
				"AnimationModifiers",       // Required for UAnimationModifier
				"AnimationBlueprintLibrary",// Required for Curve manipulation
				"AnimGraphRuntime"          // Required for FCompactPose and evaluation
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Niagara",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
