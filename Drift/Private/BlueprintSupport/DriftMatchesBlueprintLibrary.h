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

#include "DriftAPI.h"

#include "DriftMatchesBlueprintLibrary.generated.h"


UCLASS()
class UDriftMatchesBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "worldContextObject"), Category = "Drift|Matches")
    static void JoinMatch(UObject* worldContextObject, APlayerController* playerController, FBlueprintActiveMatch match);

    UFUNCTION(BlueprintPure, Category = "Drift|Matches")
    static FString GetStatus(const FBlueprintMatchQueueStatus& entry);

    UFUNCTION(BlueprintPure, Category = "Drift|Matches")
    static FBlueprintActiveMatch GetMatch(const FBlueprintMatchQueueStatus& entry);

    UFUNCTION(BlueprintPure, Category = "Drift|Matches")
    static int32 GetInvitingPlayerID(const FBlueprintMatchInvite& invite);

    UFUNCTION(BlueprintPure, meta = (WorldContext = "worldContextObject"), Category = "Drift|Matches")
    static FString GetInvitingPlayerName(UObject* worldContextObject, const FBlueprintMatchInvite& invite);
    
    UFUNCTION(BlueprintPure, Category = "Drift|Matches")
    static int32 GetExpiresInSeconds(const FBlueprintMatchInvite& invite);
};
