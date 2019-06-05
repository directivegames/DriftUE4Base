// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

using UnrealBuildTool;

public class ErrorReporter : ModuleRules
{
    public ErrorReporter(ReadOnlyTargetRules TargetRules) : base(TargetRules)
    {
        PCHUsage = PCHUsageMode.NoSharedPCHs;
        PrivatePCHHeaderFile = "Private/ErrorReporterPCH.h";

#if UE_4_19_OR_LATER
        PublicDefinitions.Add("ERROR_REPORTER_PACKAGE=1");
#else
        Definitions.Add("ERROR_REPORTER_PACKAGE=1");
#endif

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Json",
            }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
            }
            );
    }
}
