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

#pragma once

#include "DriftAPI.h"

#include "Components/ActorComponent.h"

#include "DriftEventsComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKaleoProxyStaticRoutesInitializedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKaleoProxyPlayerStatsLoadedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKaleoProxyStaticDataProgressDelegate, const FString&, name, int32, bytesRead);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKaleoProxyConnectionStateChangedDelegate, EDriftConnectionState, state);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKaleoProxyReceivedMatchInviteDelegate, const FBlueprintMatchInvite&, invite);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKaleoProxyFriendPresenceChangedDelegate, int32, playerID, EDriftPresence, presence);


struct FPlayerAuthenticatedInfo;


UCLASS(ClassGroup=Drift, HideCategories=(Activation, "Components|Activation", Collision), meta=(BlueprintSpawnableComponent))
class UDriftEventsComponent : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKaleoEventDelegate);

public:
	UDriftEventsComponent(const FObjectInitializer& oi);

	UPROPERTY(BlueprintAssignable, meta=(DisplayName="Drift Player Authenticated"))
	FKaleoEventDelegate OnPlayerAuthenticated;

    UPROPERTY(BlueprintAssignable, meta=(DisplayName="Drift Player Disconnected"))
    FKaleoEventDelegate OnPlayerDisconnected;

    UPROPERTY(BlueprintAssignable, meta=(DisplayName="Drift Connection State Changed"))
    FKaleoProxyConnectionStateChangedDelegate OnConnectionStateChanged;
    
    UPROPERTY(BlueprintAssignable, meta=(DisplayName="Drift Static Routes Initialized"))
    FKaleoProxyStaticRoutesInitializedDelegate OnStaticRoutesInitialized;

    UPROPERTY(BlueprintAssignable, meta=(DisplayName="Drift Static Data Download Progress"))
    FKaleoProxyStaticDataProgressDelegate OnStaticDataProgress;
    
    UPROPERTY(BlueprintAssignable, meta=(DisplayName="Drift Player Stats Loaded"))
    FKaleoProxyPlayerStatsLoadedDelegate OnPlayerStatsLoaded;

    UPROPERTY(BlueprintAssignable, meta=(DisplayName="Received Match Invite"))
    FKaleoProxyReceivedMatchInviteDelegate OnReceivedMatchInvite;
    
    UPROPERTY(BlueprintAssignable, meta=(DisplayName="Friend Presence Changed"))
    FKaleoProxyFriendPresenceChangedDelegate OnFriendPresenceChanged;

public:
	void OnRegister() override;
	void OnUnregister() override;

private:
    void PlayerAuthenticatedHandler(bool success, const FPlayerAuthenticatedInfo& info)
    {
        if (success)
        {
            OnPlayerAuthenticated.Broadcast();
        }
    }
    void PlayerDisconnectedHandler() { OnPlayerDisconnected.Broadcast(); }
    void ConnectionStateChangedHandler(EDriftConnectionState state) { OnConnectionStateChanged.Broadcast(state); }
    void StaticRoutesInitializedHandler() { OnStaticRoutesInitialized.Broadcast(); }
    void StaticDataProgressHandler(const FString& name, int32 bytesRead) { OnStaticDataProgress.Broadcast(name, bytesRead); }
    void PlayerStatsLoadedHandler(bool success) { OnPlayerStatsLoaded.Broadcast(); }
    void ReceivedMatchInviteHandler(const FMatchInvite& invite) { OnReceivedMatchInvite.Broadcast(FBlueprintMatchInvite{ invite }); }
    void FriendPresenceChangedHandler(int32 playerID, EDriftPresence presence) { OnFriendPresenceChanged.Broadcast(playerID, presence); }
};
