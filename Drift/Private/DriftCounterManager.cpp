// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#include "DriftPrivatePCH.h"

#include "DriftCounterManager.h"
#include "DriftSchemas.h"


DEFINE_LOG_CATEGORY(LogDriftCounters);


static const float FLUSH_COUNTERS_INTERVAL = 10.0f;


/**
 * TODO: Counters being updated before server data has been cached
 * will currently be out of sync on the client. One solution is to
 * not flush any counters before the server state has been loaded.
 */


FDriftCounterManager::FDriftCounterManager()
{
}


FString FDriftCounterManager::MakeCounterName(const FString& counterName)
{
    FString domain = TEXT("user");
    FString canonicalName = FString::Printf(TEXT("%s.%s"), *domain, *counterName);
    return canonicalName;
}


void FDriftCounterManager::AddCount(const FString& counterName, float value, bool absolute)
{
    FString canonicalName = MakeCounterName(*counterName);
    
    UE_LOG(LogDriftCounters, Verbose, TEXT("AddCount: '%s', %.2f, absolute = %s"), *canonicalName, value, absolute ? TEXT("true") : TEXT("false"));

    FCounterModification modification{ 0, value, canonicalName, absolute ? TEXT("absolute") : TEXT("count"), FDateTime::UtcNow(), absolute };
    UpdateCachedCounter(modification);

	// if this is an absolute update, we have to remove any previous relative updates
	//  otherwise future relative updates could be incorrectly consolidated into the obsolete one
	if (modification.absolute)
	{
		pendingCounters.RemoveAll([canonicalName](const FCounterModification& m)
		{
			return m.name == canonicalName && !m.absolute;
		});
	}
	// if we have a previously existing update for the same counter and type...
    int32 existingIndex;
    if (pendingCounters.Find(modification, existingIndex))
    {
		// ...we can simply update the pending modification, never sending the obsolete update to the server...
        pendingCounters[existingIndex].Update(value, FDateTime::UtcNow());
    }
    else
    {
		// ...otherwise we have to add a new pending update to the list
        pendingCounters.Add(MoveTemp(modification));
    }
}


bool FDriftCounterManager::GetCount(const FString& counterName, float& value) const
{
    FString canonicalName = MakeCounterName(*counterName);
    
    if (counterUrl.IsEmpty())
    {
        UE_LOG(LogDriftCounters, Warning, TEXT("Trying to access player counter '%s' before it has been cached!"), *canonicalName);

        return false;
    }
    
    auto counter = playerCounters.FindByPredicate([canonicalName](const FDriftPlayerCounter& pc) -> bool
    {
        return pc.name == canonicalName;
    });
    
    if (counter != nullptr)
    {
        value = counter->total;
        return true;
    }
    return false;
}


void FDriftCounterManager::LoadCounters()
{
    if (counterUrl.IsEmpty())
    {
        UE_LOG(LogDriftCounters, Warning, TEXT("Attempting to load player counters before the player session has been initialized!"));

        onPlayerStatsLoaded.Broadcast(false);
        return;
    }

    auto rm = requestManager.Pin();
    if (!rm.IsValid())
    {
        UE_LOG(LogDriftCounters, Warning, TEXT("Attempting to load player counters without a valid request manager!"));
        
        onPlayerStatsLoaded.Broadcast(false);
        return;
    }

    UE_LOG(LogDriftCounters, Log, TEXT("Getting player counters"));

    auto request = rm->Get(counterUrl);
    request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
    {
        TArray<FDriftPlayerCounter> counters;
        if (!JsonArchive::LoadObject(doc, counters))
        {
            context.error = L"Failed to parse player counter response";
            return;
        }

        playerCounters.Empty();
        for (const auto& counter : counters)
        {
            playerCounters.Add(counter);
        }
        onPlayerStatsLoaded.Broadcast(true);

        UE_LOG(LogDriftCounters, Verbose, TEXT("Got %d counters"), playerCounters.Num());
    });
    request->OnError.BindLambda([this](ResponseContext& context)
    {
        onPlayerStatsLoaded.Broadcast(false);
        context.errorHandled = true;
    });
    request->Dispatch();
}


void FDriftCounterManager::Tick(float DeltaTime)
{
    if (counterUrl.IsEmpty() || !requestManager.IsValid())
    {
        return;
    }

    flushCountersInSeconds -= DeltaTime;
    if (flushCountersInSeconds > 0.0f)
    {
        return;
    }
    
    FlushCounters();
}


TStatId FDriftCounterManager::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(FDriftCounterManager, STATGROUP_Tickables);
}


void FDriftCounterManager::FlushCounters()
{
    if (counterUrl.IsEmpty())
    {
        return;
    }

    if (pendingCounters.Num() != 0)
    {
        auto rm = requestManager.Pin();
        if (!rm.IsValid())
        {
            return;
        }
        
        UE_LOG(LogDriftCounters, Verbose, TEXT("[%s] Drift flushing %i counters..."), *FDateTime::UtcNow().ToString(), pendingCounters.Num());
        
        // TODO: thread safety?
        TArray<FCounterModification> counters{ pendingCounters };
        pendingCounters.Empty();
        auto request = rm->Put(counterUrl, counters);
        request->Dispatch();
    }
    flushCountersInSeconds += FLUSH_COUNTERS_INTERVAL;
}


void FDriftCounterManager::SetRequestManager(TSharedPtr<JsonRequestManager> newRequestManager)
{
    requestManager = newRequestManager;
    flushCountersInSeconds = FLUSH_COUNTERS_INTERVAL;
}


void FDriftCounterManager::SetCounterUrl(const FString& newCounterUrl)
{
    counterUrl = newCounterUrl;
}


/**
 * Given a counter modification, update the matching cached counter for the player
 * so that we don't need to re-download the full set of counters for every single update.
 */
void FDriftCounterManager::UpdateCachedCounter(const FCounterModification& update)
{
    auto playerCounter = playerCounters.FindByPredicate([&update](FDriftPlayerCounter& pc)
    {
        return pc.name == update.name;
    });
    if (playerCounter != nullptr)
    {
        if (update.absolute)
        {
            playerCounter->total = update.value;
        }
        else
        {
            playerCounter->total += update.value;
        }
    }
    else
    {
        playerCounters.Add(FDriftPlayerCounter(-1, update.value, update.name));
    }
}
