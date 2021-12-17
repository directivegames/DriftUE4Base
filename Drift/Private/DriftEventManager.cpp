// Copyright 2016-2019 Directive Games Limited - All Rights Reserved

#include "DriftEventManager.h"

#include "DriftSchemas.h"
#include "JsonArchive.h"

#if PLATFORM_IOS
#include "Apple/AppleUtility.h"
#endif // PLATFORM_IOS


DEFINE_LOG_CATEGORY(LogDriftEvent);
DECLARE_CYCLE_STAT(TEXT("FlushDriftEvents"), STAT_FlushDriftEvents, STATGROUP_DriftEventManager);
DECLARE_CYCLE_STAT(TEXT("ProcessDriftEvents"), STAT_ProcessDriftEvents, STATGROUP_DriftEventManager);
DECLARE_CYCLE_STAT(TEXT("UploadDriftEvents"), STAT_UploadDriftEvents, STATGROUP_DriftEventManager);

static constexpr float FLUSH_EVENTS_INTERVAL = 10.0f;
static constexpr int32 MAX_PENDING_EVENTS = 20;
static constexpr int32 MIN_SIZE_PAYLOAD_TO_COMPRESS = 200;

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

    if (pendingEvents.Num() >= MAX_PENDING_EVENTS)
    {
        UE_LOG(LogDriftEvent, Verbose, TEXT("Maximum number of pending events reached. Flushing."));

        flushEventsInSeconds = 0.0f;
        FlushEvents();
    }
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
    SCOPE_CYCLE_COUNTER(STAT_FlushDriftEvents);

    if (eventsUrl.IsEmpty())
    {
        return;
    }

    if (pendingEvents.Num() != 0)
    {
        if (!requestManager.IsValid())
        {
            UE_LOG(LogDriftEvent, Error, TEXT("Failed to flush events. Request manager is invalid."));
            return;
        }
        
        UE_LOG(LogDriftEvent, Verbose, TEXT("[%s] Drift flushing %i events..."), *FDateTime::UtcNow().ToString(), pendingEvents.Num());

        TWeakPtr<FDriftEventManager> WeakSelf = SharedThis(this);
        
        AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakSelf, Events = MoveTemp(pendingEvents)]()
        {
            SCOPE_CYCLE_COUNTER(STAT_ProcessDriftEvents);

            UE_LOG(LogDriftEvent, Verbose, TEXT("Switched to background thread for payload JSON to string processing"));

            const auto StartTime = FPlatformTime::Seconds();

            FString Payload;
            JsonArchive::SaveObject(Events, Payload);

            TArray<uint8> Compressed;
            bool bUseCompressed = false;
            const FTCHARToUTF8 Converter(*Payload);
            const auto UncompressedSize{ Converter.Length() };
            if (UncompressedSize >= MIN_SIZE_PAYLOAD_TO_COMPRESS)
            {
                UE_LOG(LogDriftEvent, Verbose, TEXT("Attempting to compress payload"));

                auto CompressedSize = FCompression::CompressMemoryBound(NAME_Gzip, UncompressedSize);
                Compressed.SetNumUninitialized(UncompressedSize);
                
                const auto Uncompressed = reinterpret_cast<const uint8*>(Converter.Get());
                
                const auto CompressionResult = FCompression::CompressMemory(NAME_Gzip, Compressed.GetData(), CompressedSize, Uncompressed, UncompressedSize);
                if (CompressionResult)
                {
                    if (CompressedSize < UncompressedSize)
                    {
                        UE_LOG(LogDriftEvent, Verbose, TEXT("Payload compression size is smaller than uncompressed size. Using compressed payload."));
                    
                        Compressed.SetNum(CompressedSize);
                        bUseCompressed = true;
                    }
                    else
                    {
                        UE_LOG(LogDriftEvent, Verbose, TEXT("Compression didn't reduce the size of the payload. Using uncompressed payload."));
                    }
                }
                else
                {
                    UE_LOG(LogDriftEvent, Verbose, TEXT("Payload compression failed. Using uncompressed payload."));
                }
            }

            const auto EndTime = FPlatformTime::Seconds();

            UE_LOG(LogDriftEvent, Verbose, TEXT("Processed '%d' events in '%.3f' seconds"), Events.Num(), EndTime - StartTime);

            AsyncTask(ENamedThreads::GameThread, [WeakSelf, Payload, Compressed, bUseCompressed]()
            {
                SCOPE_CYCLE_COUNTER(STAT_UploadDriftEvents);

                UE_LOG(LogDriftEvent, Verbose, TEXT("Switched to game thread for http request"));

                const auto PinnedSelf = WeakSelf.Pin();
                
                if (!PinnedSelf.IsValid())
                {
                    UE_LOG(LogDriftEvent, Error, TEXT("Failed to flush events. The event manager became invalid during processing."));
                    return;
                }
                
                const auto RequestManager = PinnedSelf->requestManager.Pin();
                if (!RequestManager.IsValid())
                {
                    UE_LOG(LogDriftEvent, Error, TEXT("Failed to flush events. Request manager became invalid during processing."));
                    return;
                }

                const auto Request = RequestManager->CreateRequest(HttpMethods::XPOST, PinnedSelf->eventsUrl, HttpStatusCodes::Created);

                if (bUseCompressed)
                {
                    Request->SetContent(Compressed);
                    Request->SetHeader(TEXT("Content-Encoding"), TEXT("gzip"));
                }
                else
                {
                    Request->SetPayload(Payload);
                }

                Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
                Request->Dispatch();
            });
        });
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
