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

#include "Kismet/BlueprintFunctionLibrary.h"

#include "DriftMetricsBlueprintLibrary.generated.h"


UCLASS()
class UDriftMetricsBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
     * Modify a custom counter for the currently logged in player.
     * Adds value to the counter group_name.counter_name, or if 'absolute' is true, set the counter to value.
     */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "worldContextObject"), Category = "Drift|Metrics")
	static void AddCount(UObject* worldContextObject, const FString& group_name = TEXT("default"), const FString& counter_name = TEXT(""), float value = 1.f, bool absolute = false);

    /**
     * Get the value of a custom counter.
     * The player's counters must have been successfully retrieved using LoadPlayerStats.
     */
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "worldContextObject"), Category = "Drift|Metrics")
    static void GetPlayerStat(UObject* worldContextObject, float& value, const FString& group_name = TEXT("default"), const FString& counter_name = TEXT(""));

    /**
     * Modify a custom counter for the given player. Can only be called from the server.
     * Adds value to the counter group_name.counter_name, or if 'absolute' is true, set the counter to value.
     */
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "worldContextObject"), Category = "Drift|Metrics")
    static void ModifyPlayerCounter(UObject* worldContextObject, int32 player_id, const FString& group_name = TEXT("default"), const FString& counter_name = TEXT(""), float value = 1.f, bool absolute = false);
    
    /**
     * Get the value of a player's custom counter.
     */
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "worldContextObject"), Category = "Drift|Metrics")
    static void GetPlayerCounter(UObject* worldContextObject, int32 player_id, float& value, const FString& group_name = TEXT("default"), const FString& counter_name = TEXT(""));
    
    /**
     * Flush counters, i.e. send them to the backend now, don't wait for the next automatic flush.
     * This should be used carefully, when you want a clean slate, for instance before shutting down, or
     * restarting the game session.
     */
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "worldContextObject"), Category = "Drift|Metrics")
    static void FlushCounters(UObject* worldContextObject);
};
