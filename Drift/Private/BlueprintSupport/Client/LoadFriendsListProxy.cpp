// Copyright 2016-2019 Directive Games Limited - All Rights Reserved


#include "LoadFriendsListProxy.h"

#include "DriftUtils.h"
#include "DriftBase.h"


ULoadFriendsListProxy* ULoadFriendsListProxy::LoadFriendsList(UObject* worldContextObject)
{
    auto proxy = NewObject<ULoadFriendsListProxy>();
    proxy->worldContextObject = worldContextObject;
    return proxy;
}


ULoadFriendsListProxy::ULoadFriendsListProxy(const FObjectInitializer& oi)
: Super{ oi }
{
    
}


ULoadFriendsListProxy::~ULoadFriendsListProxy()
{
    // TODO: Cancel request?
}


void ULoadFriendsListProxy::Activate()
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
    if (kc)
    {
        kc->LoadFriendsList(FDriftFriendsListLoadedDelegate::CreateUObject(this, &ThisClass::OnCompleted));
    }
}


void ULoadFriendsListProxy::OnCompleted(bool result)
{
    if (result)
    {
        OnSuccess.Broadcast();
    }
    else
    {
        OnError.Broadcast();
    }
}
