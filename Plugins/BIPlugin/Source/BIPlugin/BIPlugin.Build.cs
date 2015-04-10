using UnrealBuildTool;
using System.IO;

public class BIPlugin : ModuleRules
{
	public BIPlugin(TargetInfo Target)
	{
		PrivateIncludePaths.AddRange(
			new string[] 
			{ 
				"BIPlugin/Private" 
			});

		PublicIncludePaths.AddRange(
			new string[] 
			{ 
				"BIPlugin/Public" 
			});

		PrivateDependencyModuleNames.AddRange(
			new string[] 
		{ 
			"Core",
			"CoreUObject",
			"Engine", 
			"GraphEditor",
			"BlueprintGraph",
			"Kismet",
		});
	}
}