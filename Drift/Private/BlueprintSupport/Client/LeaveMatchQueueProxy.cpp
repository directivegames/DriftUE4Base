// Copyright 2016-2019 Directive Games Limited - All Rights Reserved


#include "LeaveMatchQueueProxy.h"

#include "DriftAPI.h"
#include "DriftUtils.h"


ULeaveMatchQueueProxy* ULeaveMatchQueueProxy::LeaveMatchQueue(UObject* worldContextObject)
{
	auto request = NewObject<ULeaveMatchQueueProxy>();
    request->worldContextObject = worldContextObject;
	return request;
}


ULeaveMatchQueueProxy::ULeaveMatchQueueProxy(const FObjectInitializer& oi)
: Super{ oi }
, worldContextObject{ nullptr }
{
}


ULeaveMatchQueueProxy::~ULeaveMatchQueueProxy()
{
    // TODO: cancel request
}


void ULeaveMatchQueueProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
	if (kc)
	{
		kc->LeaveMatchQueue(FDriftLeftMatchQueueDelegate::CreateUObject(this, &ThisClass::OnCompleted));
	}
}


void ULeaveMatchQueueProxy::OnCompleted(bool success)
{
	if (success)
	{
		OnSuccess.Broadcast(true);
	}
	else
	{
        OnError.Broadcast(false);
	}
}
