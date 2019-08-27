// Copyright 2016-2019 Directive Games Limited - All Rights Reserved


#include "AcceptMatchInviteProxy.h"

#include "DriftAPI.h"
#include "DriftUtils.h"


UAcceptMatchInviteProxy* UAcceptMatchInviteProxy::AcceptMatchInvite(UObject* worldContextObject, const FBlueprintMatchInvite& invite)
{
	auto request = NewObject<UAcceptMatchInviteProxy>();
    request->worldContextObject = worldContextObject;
    request->invite = invite;
	return request;
}


UAcceptMatchInviteProxy::UAcceptMatchInviteProxy(const FObjectInitializer& oi)
: Super{ oi }
, worldContextObject{ nullptr }
{
}


UAcceptMatchInviteProxy::~UAcceptMatchInviteProxy()
{
    // TODO: cancel request
}


void UAcceptMatchInviteProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
	if (kc)
	{
		kc->AcceptMatchInvite(invite.invite, FDriftJoinedMatchQueueDelegate::CreateUObject(this, &ThisClass::OnCompleted));
	}
}


void UAcceptMatchInviteProxy::OnCompleted(bool success, const FMatchQueueStatus& status)
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
