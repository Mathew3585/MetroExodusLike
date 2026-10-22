using UnrealBuildTool;

/** Editor-only tools for the project (never shipped in the game). */
public class MetroExodusLikeEditor : ModuleRules
{
	public MetroExodusLikeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UMG",
			"UMGEditor",
			"UnrealEd",
			"Kismet"
		});
	}
}
