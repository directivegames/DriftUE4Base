// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "DriftSchemas.h"
#include "JsonRequestManager.h"
#include "DriftAPI.h"
#include "DriftEvent.h"

#include "Tickable.h"


DECLARE_LOG_CATEGORY_EXTERN(LogDriftEvent, Log, All);


class FDriftEventManager : public FTickableGameObject
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

    void FlushEvents();

    void SetRequestManager(TSharedPtr<JsonRequestManager> newRequestManager);
    void SetEventsUrl(const FString& newEventsUrl);

private:
    TWeakPtr<JsonRequestManager> requestManager;
    FString eventsUrl;

    TArray<TUniquePtr<IDriftEvent>> pendingEvents;
    int eventSequenceIndex = 0;
    float flushEventsInSeconds = FLT_MAX;
};
