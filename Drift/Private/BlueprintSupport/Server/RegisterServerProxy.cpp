// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#include "RegisterServerProxy.h"
#include "DriftUtils.h"
#include "DriftAPI.h"


URegisterServerProxy* URegisterServerProxy::RegisterServer(UObject* worldContextObject)
{
    auto proxy = NewObject<URegisterServerProxy>();
    proxy->worldContextObject = worldContextObject;
    return proxy;
}


void URegisterServerProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto ks = helper.GetInstance();
    if (ks)
    {
        onSeverRegisteredHandle = ks->OnServerRegistered().AddUObject(this, &ThisClass::OnCompleted);
        ks->RegisterServer();
    }
}


URegisterServerProxy::~URegisterServerProxy()
{
    // TODO: Cancel the request?
}


void URegisterServerProxy::OnCompleted(bool success)
{
    FDriftWorldHelper helper{ worldContextObject };
    auto ks = helper.GetInstance();
    if (ks)
    {
        ks->OnServerRegistered().Remove(onSeverRegisteredHandle);
        onSeverRegisteredHandle.Reset();
    }
    
    if (success)
    {
        OnSuccess.Broadcast();
    }
    else
    {
        OnError.Broadcast(FDriftResponseInfo2{});
    }
}
