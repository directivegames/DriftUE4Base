
#include "LoadPlayerStatsProxy.h"

#include "DriftUtils.h"
#include "DriftBase.h"


ULoadPlayerStatsProxy* ULoadPlayerStatsProxy::LoadPlayerStats(UObject* worldContextObject)
{
    auto proxy = NewObject<ULoadPlayerStatsProxy>();
    proxy->worldContextObject = worldContextObject;
    return proxy;
}


ULoadPlayerStatsProxy::ULoadPlayerStatsProxy(const FObjectInitializer& oi)
: Super{ oi }
{
    
}


ULoadPlayerStatsProxy::~ULoadPlayerStatsProxy()
{
    // TODO: Cancel request?
}


void ULoadPlayerStatsProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
    if (kc)
    {
        onPlayerStatsLoadedHandle = kc->OnPlayerStatsLoaded().AddUObject(this, &ThisClass::OnCompleted);
        kc->LoadPlayerStats();
    }
}


void ULoadPlayerStatsProxy::OnCompleted(bool success)
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
    if (kc)
    {
        kc->OnPlayerStatsLoaded().Remove(onPlayerStatsLoadedHandle);
    }
    onPlayerStatsLoadedHandle.Reset();
    
    if (success)
    {
        OnSuccess.Broadcast();
    }
    else
    {
        OnError.Broadcast();
    }
}
