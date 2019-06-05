// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Drift : ModuleRules
{
    public Drift(ReadOnlyTargetRules TargetRules) : base(TargetRules)
    {
        bFasterWithoutUnity = true;
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
                "JsonArchive",
                "ErrorReporter",
                "Json",
            }
            );
        
        
        if (Target.Platform == UnrealTargetPlatform.IOS || Target.Platform == UnrealTargetPlatform.Mac)
        {
            // Needed for the keychain access
            PublicAdditionalFrameworks.Add(new Framework("Security"));
        }

#if UE_4_19_OR_LATER
        PublicDefinitions.Add("WITH_ANALYTICS_EVENT_ATTRIBUTE_TYPES");
#elif UE_4_18_OR_LATER
        Definitions.Add("WITH_ANALYTICS_EVENT_ATTRIBUTE_TYPES");
#endif
    }
}
