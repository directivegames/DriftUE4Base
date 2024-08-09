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
using System.IO;

public class Drift : ModuleRules
{
    public Drift(ReadOnlyTargetRules TargetRules) : base(TargetRules)
    {
#if UE_4_24_OR_LATER
		bUseUnity = false;
#else
		bFasterWithoutUnity = true;
#endif

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp17;

        PublicIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "Public")
                
                // ... add public include paths required here ...
            }
            );
                
        
        PrivateIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "Private"),

                // ... add other private include paths required here ...
            }
            );
            
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Json",
                "JsonArchive",
                "JsonUtilities"
                // ... add other public dependencies that you statically link with here ...
            }
            );
            
        
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                // ... add private dependencies that you statically link with here ...    
                "CoreUObject",
                "Engine",
                "EngineSettings",
                "Slate",
                "SlateCore",
                "HTTP",
                "Sockets",
                "RHI",
                "OnlineSubsystem",
                "OnlineSubsystemUtils",
                "DriftHttp",
                "ErrorReporter",
                "Icmp",
            }
            );
        
        
        if (Target.Platform == UnrealTargetPlatform.IOS || Target.Platform == UnrealTargetPlatform.Mac)
        {
            // Needed for the keychain access
			PublicFrameworks.AddRange(new string[]
			{
				"Security"
			});
        }

#if UE_4_19_OR_LATER
        PublicDefinitions.Add("WITH_ANALYTICS_EVENT_ATTRIBUTE_TYPES");
#elif UE_4_18_OR_LATER
        Definitions.Add("WITH_ANALYTICS_EVENT_ATTRIBUTE_TYPES");
#endif
    }
}
