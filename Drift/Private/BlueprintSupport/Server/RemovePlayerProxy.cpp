// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#include "DriftPrivatePCH.h"

#include "RemovePlayerProxy.h"
#include "DriftUtils.h"
#include "DriftAPI.h"


URemovePlayerProxy* URemovePlayerProxy::RemovePlayer(UObject* worldContextObject, int32 player_id)
{
    auto proxy = NewObject<URemovePlayerProxy>();
    proxy->player_id = player_id;
    proxy->worldContextObject = worldContextObject;
    return proxy;
}


void URemovePlayerProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto ks = helper.GetInstance();
    if (ks)
    {
        ks->RemovePlayerFromMatch(player_id, FDriftPlayerRemovedDelegate::CreateUObject(this, &ThisClass::OnCompleted));
    }
}


URemovePlayerProxy::~URemovePlayerProxy()
{
    // TODO: Cancel the request?
}


void URemovePlayerProxy::OnCompleted(bool success)
{
    if (success)
    {
        OnSuccess.Broadcast();
    }
    else
    {
        OnError.Broadcast(FDriftResponseInfo2{});
    }
}
