
#include "LoadPlayerGameStateProxy.h"

#include "DriftUtils.h"
#include "DriftBase.h"


ULoadPlayerGameStateProxy* ULoadPlayerGameStateProxy::LoadPlayerGameState(UObject* worldContextObject, FString name)
{
    auto proxy = NewObject<ULoadPlayerGameStateProxy>();
    proxy->worldContextObject_ = worldContextObject;
    proxy->name_ = name;
    return proxy;
}


ULoadPlayerGameStateProxy::ULoadPlayerGameStateProxy(const FObjectInitializer& oi)
: Super{ oi }
{
    
}


ULoadPlayerGameStateProxy::~ULoadPlayerGameStateProxy()
{
    // TODO: Cancel request?
}


void ULoadPlayerGameStateProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject_ };
    auto kc = helper.GetInstance();
    if (kc)
    {
        kc->LoadPlayerGameState(name_, FDriftGameStateLoadedDelegate::CreateUObject(this, &ThisClass::OnCompleted));
    }
}


void ULoadPlayerGameStateProxy::OnCompleted(ELoadPlayerGameStateResult result, const FString& name, const FString& state)
{
    if (name != name_)
    {
        return;
    }
    
    if (result == ELoadPlayerGameStateResult::Success)
    {
        OnSuccess.Broadcast(result, state);
    }
    else
    {
        OnError.Broadcast(result, TEXT(""));
    }
}
