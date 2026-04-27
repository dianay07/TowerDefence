// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TowerDefence : ModuleRules
{
	public TowerDefence(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"GameplayAbilities", "GameplayTags", "GameplayTasks",
			"Slate", "SlateCore", "UMG",
			// OnlineSubsystem — 세션 생성/검색/참가/퇴장 (Listen Server / Dedicated 양쪽 호환)
			"OnlineSubsystem", "OnlineSubsystemUtils",
		});

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
