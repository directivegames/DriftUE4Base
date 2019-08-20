// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#include "JoinMatchProxy.h"
#include "DriftAPI.h"
#include "DriftUtils.h"


UJoinMatchProxy* UJoinMatchProxy::JoinMatch(UObject* worldContextObject, const FBlueprintMatchInvite& invite)
{
	auto request = NewObject<UJoinMatchProxy>();
    request->worldContextObject = worldContextObject;
    request->invite = invite;
	return request;
}


UJoinMatchProxy::UJoinMatchProxy(const FObjectInitializer& oi)
: Super{ oi }
, worldContextObject{ nullptr }
{
}


UJoinMatchProxy::~UJoinMatchProxy()
{
    // TODO: cancel request
}


void UJoinMatchProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
	if (kc)
	{
		kc->JoinMatch(invite.invite, FDriftJoinedMatchQueueDelegate::CreateUObject(this, &ThisClass::OnCompleted));
	}
}


void UJoinMatchProxy::OnCompleted(bool success, const FMatchQueueStatus& status)
{
	if (success)
	{
        OnSuccess.Broadcast(FBlueprintMatchQueueStatus{ status });
	}
	else
	{
        OnError.Broadcast(FBlueprintMatchQueueStatus{});
	}
}
