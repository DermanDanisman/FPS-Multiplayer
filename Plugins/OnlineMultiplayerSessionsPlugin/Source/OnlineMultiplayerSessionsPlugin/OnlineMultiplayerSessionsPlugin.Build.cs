// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OnlineMultiplayerSessionsPlugin : ModuleRules
{
	public OnlineMultiplayerSessionsPlugin(ReadOnlyTargetRules Target) : base(Target)
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
				"DeveloperSettings",
				"UMG",
				"NetCore", 
				"EnhancedInput",
				"CommonUI",
				"CommonInput", 
				"UMG"
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"OnlineSubsystem",
				"OnlineSubsystemUtils",
				"OnlineSubsystemSteam", // NEW: Access Steam specific classes
				"Steamworks",           // NEW: Access raw Steam API
				"HTTP",
				"DeveloperSettings" // <--- ADD THIS
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
