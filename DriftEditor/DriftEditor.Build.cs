/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2016-2017 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/

namespace UnrealBuildTool.Rules
{
    public class DriftEditor : ModuleRules
    {
        public DriftEditor(ReadOnlyTargetRules TargetRules) : base(TargetRules)
        {
            PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
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