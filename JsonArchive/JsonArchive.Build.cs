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

public class JsonArchive : ModuleRules
{
    public JsonArchive(ReadOnlyTargetRules TargetRules) : base(TargetRules)
    {
        bFasterWithoutUnity = true;
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        
        PublicIncludePaths.AddRange(new string[] 
        {
        });
                
        
        PrivateIncludePaths.AddRange(new string[] 
        {
        });
            
        
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "HTTP",
			"Json",
        });


        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });
    }
}
