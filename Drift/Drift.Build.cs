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

using System.IO;
using UnrealBuildTool;

public class Drift : ModuleRules
{
    public Drift(ReadOnlyTargetRules TargetRules) : base(TargetRules)
    {
        PublicIncludePaths.AddRange(new string[] 
        {
            "Drift/Drift/Public"
        });
                
        
        PrivateIncludePaths.AddRange(new string[] 
        {
            "Drift/Drift/Private",
        });
            
        
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
        });
            
        
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Engine",
            "Slate",
            "SlateCore",
            "HTTP",
            "Sockets",
            "OnlineSubsystem",
            "OnlineSubsystemUtils",
            "DriftHttp",
            "RapidJson",
            "ErrorReporter",
            "Json",
        });
        
        
        if (Target.Platform == UnrealTargetPlatform.IOS || Target.Platform == UnrealTargetPlatform.Mac)
        {
            // Needed for the keychain access
            PublicAdditionalFrameworks.Add(new UEBuildFramework("Security"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "Launch",
                "ApplicationCore",
            });

            AdditionalPropertiesForReceipt.Add(new ReceiptProperty("AndroidPlugin", Path.Combine(ModuleDirectory, "Drift_APL.xml")));
        }
    }
}
