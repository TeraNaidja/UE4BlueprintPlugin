// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PluginProject : ModuleRules
{
	public PluginProject(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
                "AIModule",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Slate",
				"SlateCore",
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				"StrategyGame/Private/UI/Style",
			}
		);
	}
}
