
#include "DriftPrivatePCH.h"

#include "LoadStaticDataProxy.h"

#include "DriftUtils.h"
#include "DriftBase.h"


ULoadStaticDataProxy* ULoadStaticDataProxy::LoadStaticData(UObject* worldContextObject, FString name, FString ref)
{
    auto proxy = NewObject<ULoadStaticDataProxy>();
    proxy->worldContextObject = worldContextObject;
    proxy->name = name;
    proxy->ref = ref;
    return proxy;
}


ULoadStaticDataProxy::ULoadStaticDataProxy(const FObjectInitializer& oi)
: Super{ oi }
{
    
}


ULoadStaticDataProxy::~ULoadStaticDataProxy()
{
    // TODO: Cancel request?
}


void ULoadStaticDataProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
    if (kc)
    {
        onStaticDataLoadedHandle = kc->OnStaticDataLoaded().AddUObject(this, &ThisClass::OnCompleted);
        kc->LoadStaticData(name, ref);
    }
}


void ULoadStaticDataProxy::OnCompleted(bool success, const FString& data)
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
    if (kc)
    {
        kc->OnStaticDataLoaded().Remove(onStaticDataLoadedHandle);
    }
    onStaticDataLoadedHandle.Reset();
    
    if (success)
    {
        OnSuccess.Broadcast(data);
    }
    else
    {
        OnError.Broadcast(TEXT(""));
    }
}
