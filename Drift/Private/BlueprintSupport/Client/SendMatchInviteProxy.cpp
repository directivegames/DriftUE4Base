// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#include "SendMatchInviteProxy.h"
#include "DriftAPI.h"
#include "DriftUtils.h"


USendMatchInviteProxy* USendMatchInviteProxy::SendMatchInvite(UObject* worldContextObject, int32 playerID)
{
	auto request = NewObject<USendMatchInviteProxy>();
    request->worldContextObject = worldContextObject;
    request->playerID = playerID;
	return request;
}


USendMatchInviteProxy::USendMatchInviteProxy(const FObjectInitializer& oi)
: Super{ oi }
, worldContextObject{ nullptr }
{
}


USendMatchInviteProxy::~USendMatchInviteProxy()
{
    // TODO: cancel request
}


void USendMatchInviteProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
	if (kc)
	{
		kc->InvitePlayerToMatch(playerID, FDriftJoinedMatchQueueDelegate::CreateUObject(this, &ThisClass::OnCompleted));
	}
}


void USendMatchInviteProxy::OnCompleted(bool success, const FMatchQueueStatus& status)
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
