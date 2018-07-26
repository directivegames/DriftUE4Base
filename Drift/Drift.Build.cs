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
        //PCHUsage = PCHUsageMode.NoSharedPCHs;

        
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
                "RapidJson",
                "ErrorReporter",
                "Json",
            }
            );
        
        
        if (Target.Platform == UnrealTargetPlatform.IOS || Target.Platform == UnrealTargetPlatform.Mac)
        {
            // Needed for the keychain access
            PublicAdditionalFrameworks.Add(new UEBuildFramework("Security"));
        }

        BuildVersion Version;
        if (BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version))
        {
            if (Version.MajorVersion == 4 && Version.MinorVersion >= 18)
            {
                PublicDefinitions.Add("WITH_ANALYTICS_EVENT_ATTRIBUTE_TYPES");
            }
        }
    }
}
