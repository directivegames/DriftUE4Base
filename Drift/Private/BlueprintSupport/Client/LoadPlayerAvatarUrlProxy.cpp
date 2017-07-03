// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#include "DriftPrivatePCH.h"

#include "LoadPlayerAvatarUrlProxy.h"
#include "DriftAPI.h"
#include "DriftUtils.h"

ULoadPlayerAvatarUrlProxy* ULoadPlayerAvatarUrlProxy::LoadPlayerAvatarUrl(UObject* worldContextObject)
{
	auto proxy = NewObject<ULoadPlayerAvatarUrlProxy>();
	proxy->worldContextObject = worldContextObject;
	return proxy;
}

void ULoadPlayerAvatarUrlProxy::Activate()
{
	FDriftWorldHelper helper{ worldContextObject };
	auto kc = helper.GetInstance();
	if (kc)
	{
		kc->LoadPlayerAvatarUrl(FDriftLoadPlayerAvatarUrlDelegate::CreateUObject(this, &ULoadPlayerAvatarUrlProxy::OnCompleted));
	}
}

void ULoadPlayerAvatarUrlProxy::OnCompleted(const FString& url)
{
	if (url.Len())
	{
		OnSuccess.Broadcast(url);
	}
	else
	{
		OnError.Broadcast(TEXT(""));
	}
}
