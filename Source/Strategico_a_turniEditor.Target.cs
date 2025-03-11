// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Strategico_a_turniEditorTarget : TargetRules
{
    public Strategico_a_turniEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        ExtraModuleNames.Add("Strategico_a_turni");
    }
}
