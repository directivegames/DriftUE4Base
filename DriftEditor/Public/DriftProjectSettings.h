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
