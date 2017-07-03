// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#include "DriftPrivatePCH.h"

#include "SetPlayerNameProxy.h"
#include "DriftAPI.h"
#include "DriftUtils.h"


USetPlayerNameProxy* USetPlayerNameProxy::SetPlayerName(UObject* worldContextObject, APlayerController* playerController, FString name)
{
	auto proxy = NewObject<USetPlayerNameProxy>();
    proxy->name = name;
    proxy->worldContextObject = worldContextObject;
    proxy->playerController = playerController;
	return proxy;
}


USetPlayerNameProxy::USetPlayerNameProxy(const FObjectInitializer& oi)
: Super{ oi }
, worldContextObject{ nullptr }
{
}


USetPlayerNameProxy::~USetPlayerNameProxy()
{
    // TODO: cancel Proxy
}


void USetPlayerNameProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
	if (kc)
	{
		onPlayerNameSetHandle = kc->OnPlayerNameSet().AddUObject(this, &ThisClass::OnCompleted);
		kc->SetPlayerName(name);
	}
}


void USetPlayerNameProxy::OnCompleted(bool success)
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
    if (kc)
    {
        kc->OnPlayerNameSet().Remove(onPlayerNameSetHandle);
    }
    onPlayerNameSetHandle.Reset();

    if (success)
    {
        OnSuccess.Broadcast();
    }
    else
    {
        OnError.Broadcast();
    }
}
