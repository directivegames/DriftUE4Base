// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#pragma once

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
