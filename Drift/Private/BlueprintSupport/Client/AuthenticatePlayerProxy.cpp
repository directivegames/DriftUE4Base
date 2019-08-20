// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#include "AuthenticatePlayerProxy.h"
#include "DriftAPI.h"
#include "DriftUtils.h"


UAuthenticatePlayerProxy* UAuthenticatePlayerProxy::AuthenticatePlayer(UObject* worldContextObject, APlayerController* playerController)
{
	auto proxy = NewObject<UAuthenticatePlayerProxy>();
    proxy->worldContextObject = worldContextObject;
    proxy->playerController = playerController;
	return proxy;
}


UAuthenticatePlayerProxy::UAuthenticatePlayerProxy(const FObjectInitializer& oi)
: Super{ oi }
, worldContextObject{ nullptr }
{
}


UAuthenticatePlayerProxy::~UAuthenticatePlayerProxy()
{
    // TODO: cancel Proxy
}


void UAuthenticatePlayerProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
	if (kc)
	{
		onPlayerAuthenticatedHandle = kc->OnPlayerAuthenticated().AddUObject(this, &ThisClass::OnCompleted);
		kc->AuthenticatePlayer();
	}
}


void UAuthenticatePlayerProxy::OnCompleted(bool success, const FPlayerAuthenticatedInfo& info)
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
    if (kc)
    {
        kc->OnPlayerAuthenticated().Remove(onPlayerAuthenticatedHandle);
    }
    onPlayerAuthenticatedHandle.Reset();

    FDriftAuthenticatePlayerResult result{ info.playerId, info.playerName, info.result, info.error };
    if (success)
    {
        OnSuccess.Broadcast(result);
    }
    else
    {
        OnError.Broadcast(result);
    }
}
