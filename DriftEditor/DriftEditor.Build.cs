// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

namespace UnrealBuildTool.Rules
{
    public class DriftEditor : ModuleRules
    {
        public DriftEditor(ReadOnlyTargetRules TargetRules) : base(TargetRules)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "RenderCore",
                    "RHI",
                    "Slate",
                    "SlateCore",
                    "EditorStyle",
                    "EditorWidgets",
                    "DesktopWidgets",
                    "PropertyEditor",
                    "SharedSettingsWidgets",
                    "SourceControl",
                    "UnrealEd",
                    "Http",
                    "Json",
                    "JsonUtilities",
					"InputCore"
                }
            );

            DynamicallyLoadedModuleNames.AddRange(
                new string[]
                {
                    "Settings"
                }
            );

            PublicIncludePathModuleNames.AddRange(
                new string[]
                {
                    "Settings"
                }
            );
        }
    }
}