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

using UnrealBuildTool;

public class DriftHttp : ModuleRules
{
    public DriftHttp(ReadOnlyTargetRules TargetRules) : base(TargetRules)
    {
        bFasterWithoutUnity = true;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

#if UE_4_19_OR_LATER
        PublicDefinitions.Add("WITH_ENGINE_VERSION_MACROS");
        PublicDefinitions.Add("WITH_PROJECT_SAVE_DIR");
#elif UE_4_18_OR_LATER
        Definitions.Add("WITH_PROJECT_SAVE_DIR");
#endif

        PublicIncludePaths.AddRange(
            new string[]
            {
                // ... add public include paths required here ...
            }
        );


        PrivateIncludePaths.AddRange(
            new string[]
            {
                // ... add other private include paths required here ...
            }
        );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                // ... add other public dependencies that you statically link with here ...
            }
        );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                // ... add private dependencies that you statically link with here ...    
                "Engine",
                "HTTP",
                "JsonArchive",
                "ErrorReporter",
                "Json",
            }
        );
    }
}
