// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

using UnrealBuildTool;

public class JsonArchive : ModuleRules
{
    public JsonArchive(ReadOnlyTargetRules TargetRules) : base(TargetRules)
    {
        bFasterWithoutUnity = true;
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        
        PublicIncludePaths.AddRange(new string[] 
        {
        });
                
        
        PrivateIncludePaths.AddRange(new string[] 
        {
        });
            
        
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "HTTP",
			"Json",
        });


        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });
    }
}
