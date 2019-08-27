// Copyright 2016-2019 Directive Games Limited - All Rights Reserved


#include "JoinMatchQueueProxy.h"

#include "DriftAPI.h"
#include "DriftUtils.h"


UJoinMatchQueueProxy* UJoinMatchQueueProxy::JoinMatchQueue(UObject* worldContextObject)
{
	auto request = NewObject<UJoinMatchQueueProxy>();
    request->worldContextObject = worldContextObject;
	return request;
}


UJoinMatchQueueProxy::UJoinMatchQueueProxy(const FObjectInitializer& oi)
: Super{ oi }
, worldContextObject{ nullptr }
{
}


UJoinMatchQueueProxy::~UJoinMatchQueueProxy()
{
    // TODO: cancel request
}


void UJoinMatchQueueProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
	if (kc)
	{
		kc->JoinMatchQueue(FDriftJoinedMatchQueueDelegate::CreateUObject(this, &ThisClass::OnCompleted));
	}
}


void UJoinMatchQueueProxy::OnCompleted(bool success, const FMatchQueueStatus& status)
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
