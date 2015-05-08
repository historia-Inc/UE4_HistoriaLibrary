// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class JsonSpawnModule : ModuleRules
	{
		public JsonSpawnModule(TargetInfo Target)
		{
			DynamicallyLoadedModuleNames.AddRange(
				new string[] {
				}
				);

			PublicIncludePaths.AddRange(
				new string[] {
					"JsonSpawnModule/Public",
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"JsonSpawnModule/Private",
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"Json",
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