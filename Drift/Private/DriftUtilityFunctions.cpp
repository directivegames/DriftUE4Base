// Copyright (C) 2024-2025 - Directive Games Limited - All Rights Reserved


#include "DriftUtilityFunctions.h"


TArray<FString> UDriftUtilityFunctions::GetGameVersionStringSplit()
{
    FString GameVersion;
    GConfig->GetString(TEXT("/Script/DriftEditor.DriftProjectSettings"), TEXT("GameVersion"), GameVersion, GGameIni);

    if (!GameVersion.IsEmpty())
    {
        FString Major;
        FString Minor;
        FString Patch;
        GameVersion.Split(".", &Major, &Minor);
        Minor.Split(".", &Minor, &Patch);

        FString GameBuild;
        GConfig->GetString(TEXT("/Script/DriftEditor.DriftProjectSettings"), TEXT("GameBuild"), GameBuild, GGameIni);

        if (GameBuild.IsEmpty())
        {
            GameBuild = "0";
        }

        return { Major, Minor, Patch, GameBuild };
    }

    return { "0", "0", "0", "0" };
}

FString UDriftUtilityFunctions::GetGameVersionString()
{
    const auto Split = GetGameVersionStringSplit();

    return Split[0] + "." + Split[1] + "." + Split[2] + "-" + Split[3];
}
