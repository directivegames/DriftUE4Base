// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#include "DriftPrivatePCH.h"

#include "DriftEventManager.h"
#include "DriftSchemas.h"
#include "JsonArchive.h"

#if PLATFORM_IOS
#include "Apple/AppleUtility.h"
#endif // PLATFORM_IOS


DEFINE_LOG_CATEGORY(LogDriftEvent);


static const float FLUSH_EVENTS_INTERVAL = 10.0f;


FDriftEventManager::FDriftEventManager()
{
    InitDefaultTags();
}


void FDriftEventManager::AddEvent(TUniquePtr<IDriftEvent> event)
{
    UE_LOG(LogDriftEvent, Verbose, TEXT("Adding event: %s"), *event->GetName());

    event->Add(TEXT("sequence"), ++eventSequenceIndex);
    AddTags(event);
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


void FDriftEventManager::InitDefaultTags()
{
    tags_.FindOrAdd(TEXT("device_model")) = FPlatformMisc::GetDefaultDeviceProfileName();

    FString gameVersion;
    GConfig->GetString(TEXT("/Script/DriftEditor.DriftProjectSettings"), TEXT("GameVersion"), gameVersion, GGameIni);
    tags_.FindOrAdd(TEXT("client_version")) = gameVersion;
    FString gameBuild;
    GConfig->GetString(TEXT("/Script/DriftEditor.DriftProjectSettings"), TEXT("GameBuild"), gameBuild, GGameIni);
    tags_.FindOrAdd(TEXT("client_build")) = gameBuild;

#if PLATFORM_IOS
    tags_.FindOrAdd(TEXT("os_version")) = IOSUtility::GetIOSVersion();
    tags_.FindOrAdd(TEXT("os_build")) = IOSUtility::GetIOSBuild();
    tags_.FindOrAdd(TEXT("device_model_id")) = IOSUtility::GetHardwareModel();
#endif // PLATFORM_IOS
}


void FDriftEventManager::AddTags(const TUniquePtr<IDriftEvent>& event)
{
    for (const auto& tag : tags_)
    {
        event->Add(tag.Key, tag.Value);
    }
}
