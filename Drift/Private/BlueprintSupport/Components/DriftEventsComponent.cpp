/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2016-2017 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/

#include "DriftEventsComponent.h"
#include "DriftAPI.h"
#include "DriftUtils.h"


UDriftEventsComponent::UDriftEventsComponent(const FObjectInitializer& oi)
: Super{ oi }
{

}


void UDriftEventsComponent::OnRegister()
{
	Super::OnRegister();

    auto drift = FDriftWorldHelper{ GetWorld() }.GetInstance();
	if (drift)
	{
        drift->OnPlayerAuthenticated().AddUObject(this, &ThisClass::PlayerAuthenticatedHandler);
        drift->OnPlayerDisconnected().AddUObject(this, &ThisClass::PlayerDisconnectedHandler);
        drift->OnConnectionStateChanged().AddUObject(this, &ThisClass::ConnectionStateChangedHandler);
        drift->OnStaticRoutesInitialized().AddUObject(this, &ThisClass::StaticRoutesInitializedHandler);
        drift->OnStaticDataProgress().AddUObject(this, &ThisClass::StaticDataProgressHandler);
        drift->OnPlayerStatsLoaded().AddUObject(this, &ThisClass::PlayerStatsLoadedHandler);
        drift->OnReceivedMatchInvite().AddUObject(this, &ThisClass::ReceivedMatchInviteHandler);
        drift->OnFriendPresenceChanged().AddUObject(this, &ThisClass::FriendPresenceChangedHandler);
   	}
}


void UDriftEventsComponent::OnUnregister()
{
	Super::OnUnregister();

    auto drift = FDriftWorldHelper{ GetWorld() }.GetInstance();
    if (drift)
    {
        drift->OnPlayerAuthenticated().RemoveAll(this);
        drift->OnPlayerDisconnected().RemoveAll(this);
        drift->OnConnectionStateChanged().RemoveAll(this);
        drift->OnStaticRoutesInitialized().RemoveAll(this);
        drift->OnStaticDataProgress().RemoveAll(this);
        drift->OnPlayerStatsLoaded().RemoveAll(this);
        drift->OnReceivedMatchInvite().RemoveAll(this);
        drift->OnFriendPresenceChanged().RemoveAll(this);
    }
}
