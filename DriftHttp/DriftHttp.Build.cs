// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

using UnrealBuildTool;

public class DriftHttp : ModuleRules
{
    public DriftHttp(ReadOnlyTargetRules TargetRules) : base(TargetRules)
    {
        bFasterWithoutUnity = true;
        PCHUsage = PCHUsageMode.NoSharedPCHs;

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
