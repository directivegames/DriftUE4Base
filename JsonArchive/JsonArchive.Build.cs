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
#if UE_4_24_OR_LATER
		bUseUnity = false;
#else
		bFasterWithoutUnity = true;
#endif

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp17;
        
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

        if (TargetRules.bBuildEditor == true)
        {
	        PrivateDependencyModuleNames.Add("UnrealEd");
        }
    }
}
