// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#include "DriftPrivatePCH.h"

#include "AddPlayerProxy.h"
#include "DriftUtils.h"
#include "DriftAPI.h"


UAddPlayerProxy* UAddPlayerProxy::AddPlayer(UObject* worldContextObject, int32 player_id, int32 team_id)
{
    auto proxy = NewObject<UAddPlayerProxy>();
    proxy->player_id = player_id;
    proxy->team_id = team_id;
    proxy->worldContextObject = worldContextObject;
    return proxy;
}


void UAddPlayerProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto ks = helper.GetInstance();
    if (ks)
    {
        ks->AddPlayerToMatch(player_id, team_id, FDriftPlayerAddedDelegate::CreateUObject(this, &ThisClass::OnCompleted));
    }
}


UAddPlayerProxy::~UAddPlayerProxy()
{
    // TODO: Cancel the request?
}


void UAddPlayerProxy::OnCompleted(bool success)
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
