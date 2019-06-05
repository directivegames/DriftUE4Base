// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#pragma once

#include "DriftProjectSettings.generated.h"


UCLASS(Config=Game, DefaultConfig)
class UDriftProjectSettings : public UObject
{
    GENERATED_BODY()
    
public:
    /** The API key for this project. */
    UPROPERTY(Config, EditAnywhere)
    FString ApiKey;

    /** When using dedicated servers, the named reference to match against. */
    UPROPERTY(Config, EditAnywhere)
    FString BuildReference;
    
    /** The git ref to match when loading static data. */
    UPROPERTY(Config, EditAnywhere)
    FString StaticDataReference;
    
    /** When using dedicated servers, the location to match against. */
    UPROPERTY(Config, EditAnywhere)
    FString Placement;

    /** The project name. */
    UPROPERTY(Config, EditAnywhere)
    FString ProjectName;

    /** The version of the game. We recommend following semver.org. */
    UPROPERTY(Config, EditAnywhere)
    FString GameVersion;
    
    /** The build number distinguishing two builds of the same version. */
    UPROPERTY(Config, EditAnywhere)
    FString GameBuild;

    /** Target environment, such as 'dev', 'test', and 'live'. */
    UPROPERTY(Config, EditAnywhere)
    FString Environment;

    /** URL for your Drift tenant root. */
    UPROPERTY(Config, EditAnywhere)
    FString DriftUrl;

    UDriftProjectSettings(const FObjectInitializer& ObjectInitializer);
};
