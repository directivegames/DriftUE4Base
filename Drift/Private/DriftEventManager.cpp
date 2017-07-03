// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#include "DriftPrivatePCH.h"

#include "DriftEventManager.h"
#include "DriftSchemas.h"
#include "JsonArchive.h"


DEFINE_LOG_CATEGORY(LogDriftEvent);


static const float FLUSH_EVENTS_INTERVAL = 10.0f;


FDriftEventManager::FDriftEventManager()
{
}


void FDriftEventManager::AddEvent(TUniquePtr<IDriftEvent> event)
{
    UE_LOG(LogDriftEvent, Verbose, TEXT("Adding event: %s"), *event->GetName());

    event->Add(TEXT("sequence"), ++eventSequenceIndex);
    pendingEvents.Add(MoveTemp(event));
}


void FDriftEventManager::Tick(float DeltaTime)
{
    if (eventsUrl.IsEmpty() || !requestManager.IsValid())
    {
        return;
    }

    flushEventsInSeconds -= DeltaTime;
    if (flushEventsInSeconds > 0.0f)
    {
        return;
    }
    
    FlushEvents();
}


TStatId FDriftEventManager::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(FDriftEventManager, STATGROUP_Tickables);
}


void FDriftEventManager::FlushEvents()
{
    if (eventsUrl.IsEmpty())
    {
        return;
    }

    if (pendingEvents.Num() != 0)
    {
        auto rm = requestManager.Pin();
        if (rm.IsValid())
        {
            UE_LOG(LogDriftEvent, Verbose, TEXT("[%s] Drift flushing %i events..."), *FDateTime::UtcNow().ToString(), pendingEvents.Num());

            FString payload;
            JsonArchive::SaveObject(pendingEvents, payload);

            pendingEvents.Empty();
            auto request = rm->Post(eventsUrl, payload);
            request->Dispatch();
            
        }
    }
    flushEventsInSeconds += FLUSH_EVENTS_INTERVAL;
}


void FDriftEventManager::SetRequestManager(TSharedPtr<JsonRequestManager> newRequestManager)
{
    requestManager = newRequestManager;
}


void FDriftEventManager::SetEventsUrl(const FString& newEventsUrl)
{
    eventsUrl = newEventsUrl;
    flushEventsInSeconds = FLUSH_EVENTS_INTERVAL;
}
