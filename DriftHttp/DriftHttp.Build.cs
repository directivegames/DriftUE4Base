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

        PublicIncludePaths.AddRange(new string[]
		{
		});


        PrivateIncludePaths.AddRange(new string[]
		{
		});


        PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"JsonArchive",
		});


        PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Engine",
			"HTTP",
			"ErrorReporter",
			"Json",
		});
    }
}
