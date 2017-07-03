// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#include "DriftPrivatePCH.h"

#include "GetActiveMatchesProxy.h"
#include "DriftAPI.h"
#include "DriftUtils.h"


UGetActiveMatchesProxy* UGetActiveMatchesProxy::GetActiveMatches(UObject* worldContextObject, FString ref_filter)
{
	auto request = NewObject<UGetActiveMatchesProxy>();
	request->ref_filter = ref_filter;
    request->worldContextObject = worldContextObject;
	return request;
}


FString UGetActiveMatchesProxy::GetGameMode(const FBlueprintActiveMatch& entry)
{
    return entry.match.game_mode;
}


FString UGetActiveMatchesProxy::GetMapName(const FBlueprintActiveMatch& entry)
{
    return entry.match.map_name;
}


FString UGetActiveMatchesProxy::GetMatchStatus(const FBlueprintActiveMatch& entry)
{
    return entry.match.match_status;
}


int32 UGetActiveMatchesProxy::GetNumPlayers(const FBlueprintActiveMatch& entry)
{
    return entry.match.num_players;
}


FString UGetActiveMatchesProxy::GetServerStatus(const FBlueprintActiveMatch& entry)
{
    return entry.match.server_status;
}


UGetActiveMatchesProxy::UGetActiveMatchesProxy(const FObjectInitializer& oi)
: Super{ oi }
, worldContextObject{ nullptr }
{
}


UGetActiveMatchesProxy::~UGetActiveMatchesProxy()
{
    // TODO: cancel request
}


void UGetActiveMatchesProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
	if (kc)
	{
        search = MakeShareable(new FMatchesSearch());
		onGotActiveMatchesHandle = kc->OnGotActiveMatches().AddUObject(this, &ThisClass::OnCompleted);
        search->ref_filter = ref_filter;
		kc->GetActiveMatches(search.ToSharedRef());
	}
}


void UGetActiveMatchesProxy::OnCompleted(bool success)
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
	if (kc)
	{
        kc->OnGotActiveMatches().Remove(onGotActiveMatchesHandle);
        onGotActiveMatchesHandle.Reset();
	}

	TArray<FBlueprintActiveMatch> matches;
	if (success && search.IsValid())
	{
		for (const auto& result : search->matches)
		{
			FBlueprintActiveMatch match;
            match.match = result;
			matches.Add(match);
		}
		OnSuccess.Broadcast(matches);
	}
	else
	{
        OnError.Broadcast(matches);
	}
}
