// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#include "DriftPrivatePCH.h"

#include "PollMatchQueueProxy.h"
#include "DriftAPI.h"
#include "DriftUtils.h"


UPollMatchQueueProxy* UPollMatchQueueProxy::PollMatchQueue(UObject* worldContextObject)
{
	auto request = NewObject<UPollMatchQueueProxy>();
    request->worldContextObject = worldContextObject;
	return request;
}


UPollMatchQueueProxy::UPollMatchQueueProxy(const FObjectInitializer& oi)
: Super{ oi }
, worldContextObject{ nullptr }
{
}


UPollMatchQueueProxy::~UPollMatchQueueProxy()
{
    // TODO: cancel request
}


void UPollMatchQueueProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
	if (kc)
	{
        kc->PollMatchQueue(FDriftPolledMatchQueueDelegate::CreateUObject(this, &ThisClass::OnCompleted));
	}
}


void UPollMatchQueueProxy::OnCompleted(bool success, const FMatchQueueStatus& match)
{
	if (success)
	{
		OnSuccess.Broadcast(FBlueprintMatchQueueStatus{ match });
	}
	else
	{
        OnError.Broadcast(FBlueprintMatchQueueStatus{});
	}
}
