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

public class RapidJson : ModuleRules
{
    public RapidJson(ReadOnlyTargetRules TargetRules) : base(TargetRules)
    {
        bFasterWithoutUnity = true;
        PCHUsage = PCHUsageMode.NoSharedPCHs;

        
        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
            );
                
        
        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
            }
            );
            
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                // ... add other public dependencies that you statically link with here ...
                "Core",
                "HTTP",
            }
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                // ... add private dependencies that you statically link with here ...    
            }
            );

        if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.PS4))
        {
            PublicDefinitions.Add("RAPIDJSON_HAS_CXX11_RVALUE_REFS=1");
        }
    }
}
