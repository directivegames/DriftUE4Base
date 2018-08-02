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
