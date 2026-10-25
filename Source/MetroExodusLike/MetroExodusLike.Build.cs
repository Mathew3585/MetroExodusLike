// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MetroExodusLike : ModuleRules
{
	public MetroExodusLike(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"PhysicsCore",       // Physical materials (footsteps, impacts)
			"AIModule",          // AI controllers, perception, behavior trees
			"GameplayTasks",     // Required by AIModule tasks
			"NavigationSystem",  // Navmesh queries
			"UMG",               // Widgets (prompts, menus)
			"Niagara",           // Effects (impacts, muzzle flash)
			"AnimGraphRuntime",  // Custom anim nodes (arms offset)
			"StateTreeModule",   // AI logic (StateTree)
			"GameplayStateTreeModule" // StateTree on AI controllers
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore"
		});

		// Lets us include headers by folder: #include "Player/MECharacter.h"
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
