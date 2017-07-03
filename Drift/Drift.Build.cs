// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

using UnrealBuildTool;

public class Drift : ModuleRules
{
    public Drift(ReadOnlyTargetRules TargetRules) : base(TargetRules)
    {
        bFasterWithoutUnity = true;
        //PCHUsage = PCHUsageMode.NoSharedPCHs;

        
        PublicIncludePaths.AddRange(
            new string[] {
                "Drift/Drift/Public"
                
                // ... add public include paths required here ...
            }
            );
                
        
        PrivateIncludePaths.AddRange(
            new string[] {
                "Drift/Drift/Private",

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
            }
            );
        
        
        if (Target.Platform == UnrealTargetPlatform.IOS || Target.Platform == UnrealTargetPlatform.Mac)
        {
            // Needed for the keychain access
            PublicAdditionalFrameworks.Add(new UEBuildFramework("Security"));
        }
    }
}
