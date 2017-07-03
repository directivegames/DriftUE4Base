// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

using UnrealBuildTool;

public class ErrorReporter : ModuleRules
{
    public ErrorReporter(ReadOnlyTargetRules TargetRules) : base(TargetRules)
    {
        PCHUsage = PCHUsageMode.NoSharedPCHs;
        PrivatePCHHeaderFile = "Private/ErrorReporterPCH.h";
        
        Definitions.Add("ERROR_REPORTER_PACKAGE=1");

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
