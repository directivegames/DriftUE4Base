// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "DriftSchemas.h"
#include "JsonRequestManager.h"
#include "DriftAPI.h"
#include "DriftEvent.h"

#include "Tickable.h"


DECLARE_LOG_CATEGORY_EXTERN(LogDriftEvent, Log, All);
DECLARE_STATS_GROUP(TEXT("Drift Event Manager"), STATGROUP_DriftEventManager, STATCAT_Advanced);

class FDriftEventManager : public FTickableGameObject, public TSharedFromThis<FDriftEventManager>
{
public:
    FDriftEventManager();

    /**
     * FTickableGameObject overrides
     */
    void Tick(float DeltaTime) override;
    bool IsTickable() const override { return true; }

    TStatId GetStatId() const override;

    /**
     * API
     */
    void AddEvent(TUniquePtr<IDriftEvent> event);

    void LoadCounters();

    void FlushEvents(bool bSynchronous = false);

    void SetRequestManager(TSharedPtr<JsonRequestManager> newRequestManager);
    void SetEventsUrl(const FString& newEventsUrl);

private:
    void InitDefaultTags();
    void AddTags(const TUniquePtr<IDriftEvent>& event);

    void FlushEventsInternal();
    void FlushEventsInternalAsync();

    static void ProcessEvents(const TArray<TUniquePtr<IDriftEvent>>& Events, FString& Payload, TArray<uint8>& Compressed, bool& bUseCompressed);
    static void ProcessRequest(const TSharedPtr<JsonRequestManager> RequestManager, const FString& URL, const FString& Payload, const TArray<uint8>& Compressed, bool bUseCompressed);

private:
    TWeakPtr<JsonRequestManager> requestManager;
    FString eventsUrl;

    TArray<TUniquePtr<IDriftEvent>> pendingEvents;
    int eventSequenceIndex = 0;
    float flushEventsInSeconds = FLT_MAX;

    TMap<FString, FString> tags_;
};
