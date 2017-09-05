// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#include "DriftPrivatePCH.h"

#include "RemovePlayerProxy.h"
#include "DriftUtils.h"
#include "DriftAPI.h"


URemovePlayerProxy* URemovePlayerProxy::RemovePlayer(UObject* worldContextObject, int32 player_id)
{
    auto proxy = NewObject<URemovePlayerProxy>();
    proxy->player_id_ = player_id;
    proxy->worldContextObject_ = worldContextObject;
    return proxy;
}


void URemovePlayerProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject_ };
    auto ks = helper.GetInstance();
    if (ks)
    {
        ks->RemovePlayerFromMatch(player_id_, FDriftPlayerRemovedDelegate::CreateUObject(this, &ThisClass::OnCompleted));
    }
}


URemovePlayerProxy::~URemovePlayerProxy()
{
    // TODO: Cancel the request?
}


void URemovePlayerProxy::OnCompleted(bool success, int32 match_id, int32 player_id)
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
