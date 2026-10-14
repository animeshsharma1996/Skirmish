// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Skirmish : ModuleRules
{
	public Skirmish(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] 
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] 
		{
			"Skirmish",
			"Skirmish/Variant_Platforming",
			"Skirmish/Variant_Platforming/Animation",
			"Skirmish/Variant_Combat",
			"Skirmish/Variant_Combat/AI",
			"Skirmish/Variant_Combat/Animation",
			"Skirmish/Variant_Combat/Gameplay",
			"Skirmish/Variant_Combat/Interfaces",
			"Skirmish/Variant_Combat/UI",
			"Skirmish/Variant_SideScrolling",
			"Skirmish/Variant_SideScrolling/AI",
			"Skirmish/Variant_SideScrolling/Gameplay",
			"Skirmish/Variant_SideScrolling/Interfaces",
			"Skirmish/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
