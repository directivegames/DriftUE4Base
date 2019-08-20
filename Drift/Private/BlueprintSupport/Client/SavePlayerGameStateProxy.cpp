
#include "SavePlayerGameStateProxy.h"

#include "DriftUtils.h"
#include "DriftBase.h"


USavePlayerGameStateProxy* USavePlayerGameStateProxy::LoadStaticData(UObject* worldContextObject, FString name, FString state)
{
    auto proxy = NewObject<USavePlayerGameStateProxy>();
    proxy->worldContextObject = worldContextObject;
    proxy->name_ = name;
    proxy->state_ = state;
    return proxy;
}


USavePlayerGameStateProxy::USavePlayerGameStateProxy(const FObjectInitializer& oi)
: Super{ oi }
{
    
}


USavePlayerGameStateProxy::~USavePlayerGameStateProxy()
{
    // TODO: Cancel request?
}


void USavePlayerGameStateProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
    if (kc)
    {
        kc->SavePlayerGameState(name_, state_, FDriftGameStateSavedDelegate::CreateUObject(this, &ThisClass::OnCompleted));
    }
}


void USavePlayerGameStateProxy::OnCompleted(bool success, const FString& name)
{
    if (name != name_)
    {
        return;
    }

    if (success)
    {
        OnSuccess.Broadcast();
    }
    else
    {
        OnError.Broadcast();
    }
}
