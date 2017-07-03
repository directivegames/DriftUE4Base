// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "DriftSchemas.h"
#include "JsonRequestManager.h"
#include "DriftAPI.h"

#include "Tickable.h"


DECLARE_LOG_CATEGORY_EXTERN(LogDriftCounters, Log, All);


class FDriftCounterManager : public FTickableGameObject
{
public:
    static FString MakeCounterName(const FString& counterName);

    FDriftCounterManager();

    /**
     * FTickableGameObject overrides
     */
    void Tick(float DeltaTime) override;
    bool IsTickable() const override { return true; }

    TStatId GetStatId() const override;

    /**
     * API
     */
    void AddCount(const FString& counterName, float value, bool absolute);
    bool GetCount(const FString& counterName, float& value) const;

    void LoadCounters();

    void FlushCounters();

    void SetRequestManager(TSharedPtr<JsonRequestManager> newRequestManager);
    void SetCounterUrl(const FString& newCounterUrl);

    FDriftPlayerStatsLoadedDelegate& OnPlayerStatsLoaded() { return onPlayerStatsLoaded; }

private:
    void UpdateCachedCounter(const FCounterModification& update);

    FDriftPlayerStatsLoadedDelegate onPlayerStatsLoaded;

private:
    TWeakPtr<JsonRequestManager> requestManager;
    FString counterUrl;

    TArray<FCounterModification> pendingCounters;
    float flushCountersInSeconds = FLT_MAX;

    TArray<FDriftPlayerCounter> playerCounters;
};
