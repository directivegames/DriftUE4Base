// Copyright 2016-2019 Directive Games Limited - All Rights Reserved


#include "AddMatchProxy.h"

#include "DriftUtils.h"
#include "DriftAPI.h"


UAddMatchProxy* UAddMatchProxy::AddMatch(UObject* worldContextObject, int32 max_players, FString map_name, FString game_mode, int32 num_teams)
{
	auto proxy = NewObject<UAddMatchProxy>();
	proxy->max_players = max_players;
	proxy->map_name = map_name;
	proxy->game_mode = game_mode;
	proxy->num_teams = num_teams;
    proxy->worldContextObject = worldContextObject;
	return proxy;
}


void UAddMatchProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto ks = helper.GetInstance();
	if (ks)
	{
        onMatchAddedHandle = ks->OnMatchAdded().AddUObject(this, &ThisClass::OnCompleted);
		ks->AddMatch(map_name, game_mode, num_teams, max_players);
	}
}


UAddMatchProxy::UAddMatchProxy()
{
	// TODO: Cancel the request?
}


void UAddMatchProxy::OnCompleted(bool success)
{
    FDriftWorldHelper helper{ worldContextObject };
    auto ks = helper.GetInstance();
    if (ks)
    {
        ks->OnMatchAdded().Remove(onMatchAddedHandle);
        onMatchAddedHandle.Reset();
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
