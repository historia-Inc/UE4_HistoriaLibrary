// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class JsonPropModule : ModuleRules
	{
		public JsonPropModule(TargetInfo Target)
		{
			DynamicallyLoadedModuleNames.AddRange(
				new string[] {
				}
				);

			PublicIncludePaths.AddRange(
				new string[] {
					"JsonPropModule/Public",
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"JsonPropModule/Private",
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"UnrealEd",
					"ContentBrowser",
					"SlateCore",
					"Slate",
					"Json",
					"DesktopPlatform",
					"MainFrame",
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
				}
				);
		}
	}
}