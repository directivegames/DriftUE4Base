// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#include "DriftPrivatePCH.h"

#include "UpdateMatchProxy.h"
#include "DriftUtils.h"
#include "DriftAPI.h"


UUpdateMatchProxy* UUpdateMatchProxy::UpdateMatch(UObject* worldContextObject, FString match_url, FString status)
{
	auto proxy = NewObject<UUpdateMatchProxy>();
	proxy->status = status;
    proxy->worldContextObject = worldContextObject;
	return proxy;
}


void UUpdateMatchProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto ks = helper.GetInstance();
	if (ks)
	{
        ks->UpdateMatch(status, TEXT(""), FDriftMatchStatusUpdatedDelegate{});
        return;
	}
}


UUpdateMatchProxy::~UUpdateMatchProxy()
{
	// TODO: Cancel the request?
}
