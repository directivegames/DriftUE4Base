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
#include "DriftSchemas.h"
#include "CommonDelegates.h"
#include "JsonRequestManager.h"
#include "DriftCounterManager.h"
#include "DriftEventManager.h"
#include "DriftMessageQueue.h"
#include "LogForwarder.h"

#include "Tickable.h"


class ResponseContext;
class FKaleoErrorDelegate;


DECLARE_LOG_CATEGORY_EXTERN(LogDriftBase, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogDriftCounterEngine, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogDriftCounterUser, Log, All);


#define DRIFT_LOG(Channel, Verbosity, Format, ...) \
{ \
    UE_LOG(LogDrift##Channel, Verbosity, TEXT("%s") Format, *instanceDisplayName_, ##__VA_ARGS__); \
}


class UGetActiveMatchesRequest;


enum class DriftSessionState
{
    Undefined,
    Disconnected,
    Connecting,
    Connected,
    Disconnecting,
    Usurped,
    Timedout,
};


class IDriftAuthProviderFactory;
class IDriftAuthProvider;


class FDriftBase : public IDriftAPI, public FTickableGameObject
{
public:
    FDriftBase(TSharedPtr<IHttpCache> cache, const FName& instanceName, int32 instanceIndex);
    FDriftBase(const FDriftBase& other) = delete;
    virtual ~FDriftBase();

    // FTickableGameObject API
    void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return true; }

    TStatId GetStatId() const;

    // Generic API
    void AuthenticatePlayer() override;
    EDriftConnectionState GetConnectionState() const override;
    FString GetPlayerName() override;
    int32 GetPlayerID() override;
    void SetPlayerName(const FString& name) override;
    FString GetAuthProviderName() const override;
    void AddPlayerIdentity(const FString& authProvider, const FDriftAddPlayerIdentityProgressDelegate& progressDelegate) override;

    void GetActiveMatches(const TSharedRef<FMatchesSearch>& search) override;
    void JoinMatchQueue(const FDriftJoinedMatchQueueDelegate& delegate) override;
    void LeaveMatchQueue(const FDriftLeftMatchQueueDelegate& delegate) override;
    void PollMatchQueue(const FDriftPolledMatchQueueDelegate& delegate) override;
    void ResetMatchQueue() override;
    EMatchQueueState GetMatchQueueState() const override;
    void InvitePlayerToMatch(int32 playerId, const FDriftJoinedMatchQueueDelegate& delegate) override;
    void JoinMatch(const FMatchInvite& invite, const FDriftJoinedMatchQueueDelegate& delegate) override;
    void AcceptMatchInvite(const FMatchInvite& invite, const FDriftJoinedMatchQueueDelegate& delegate) override;

    void AddCount(const FString& counter_name, float value, bool absolute) override;
    bool GetCount(const FString& counter_name, float& value) override;

    void AddAnalyticsEvent(const FString& event_name, const TArray<FAnalyticsEventAttribute>& attributes) override;
    void AddAnalyticsEvent(TUniquePtr<IDriftEvent> event) override;

    void LoadStaticData(const FString& name, const FString& ref) override;

    void LoadPlayerStats() override;

    void LoadPlayerGameState(const FString& name, const FDriftGameStateLoadedDelegate& delegate) override;
    void SavePlayerGameState(const FString& name, const FString& gameState, const FDriftGameStateSavedDelegate& delegate) override;

    void GetLeaderboard(const FString& counter_name, const TSharedRef<FDriftLeaderboard>& leaderboard, const FDriftLeaderboardLoadedDelegate& delegate) override;
    void GetFriendsLeaderboard(const FString& counter_name, const TSharedRef<FDriftLeaderboard>& leaderboard, const FDriftLeaderboardLoadedDelegate& delegate) override;

    void LoadFriendsList(const FDriftFriendsListLoadedDelegate& delegate) override;
    void UpdateFriendsList() override;
    bool GetFriendsList(TArray<FDriftFriend>& friends) override;
    FString GetFriendName(int32 friendID) override;
    void LoadPlayerAvatarUrl(const FDriftLoadPlayerAvatarUrlDelegate& delegate) override;

    void FlushCounters() override;
    void FlushEvents() override;

    void Shutdown() override;
    
    FDriftPlayerAuthenticatedDelegate& OnPlayerAuthenticated() override { return onPlayerAuthenticated; }
    FDriftConnectionStateChangedDelegate& OnConnectionStateChanged() override { return onConnectionStateChanged; }
    FDriftFriendPresenceChangedDelegate& OnFriendPresenceChanged() override { return onFriendPresenceChanged; }
    FDriftRecievedMatchInviteDelegate& OnReceivedMatchInvite() override { return onReceivedMatchInvite; }
    FDriftStaticDataLoadedDelegate& OnStaticDataLoaded() override { return onStaticDataLoaded; }
    FDriftStaticDataProgressDelegate& OnStaticDataProgress() override { return onStaticDataProgress; }
    FDriftPlayerStatsLoadedDelegate& OnPlayerStatsLoaded() override { return onPlayerStatsLoaded; }
    FDriftPlayerGameStateLoadedDelegate& OnPlayerGameStateLoaded() { return onPlayerGameStateLoaded; }
    FDriftPlayerGameStateSavedDelegate& OnPlayerGameStateSaved() { return onPlayerGameStateSaved; }
    FDriftGotActiveMatchesDelegate& OnGotActiveMatches() override { return onGotActiveMatches; }
    FDriftPlayerNameSetDelegate& OnPlayerNameSet() override { return onPlayerNameSet; }
    FDriftStaticRoutesInitializedDelegate& OnStaticRoutesInitialized() override { return onStaticRoutesInitialized; }
    FDriftPlayerDisconnectedDelegate& OnPlayerDisconnected() override { return onPlayerDisconnected; }
    FDriftGameVersionMismatchDelegate& OnGameVersionMismatch() override { return onGameVersionMismatch; }
    FDriftUserErrorDelegate& OnUserError() override { return onUserError; }
    FDriftServerErrorDelegate& OnServerError() override { return onServerError; }

    // Server API
    bool RegisterServer() override;
    
    void AddMatch(const FString& map_name, const FString& game_mode, int32 num_teams, int32 max_players) override;
    void UpdateServer(const FString& status, const FString& reason, const FDriftServerStatusUpdatedDelegate& delegate) override;
    void UpdateMatch(const FString& status, const FString& reason, const FDriftMatchStatusUpdatedDelegate& delegate) override;
    int32 GetMatchID() const override;
    void AddPlayerToMatch(int32 player_id, int32 team_id, const FDriftPlayerAddedDelegate& delegate) override;
    void RemovePlayerFromMatch(int32 player_id, const FDriftPlayerRemovedDelegate& delegate) override;
    void ModifyPlayerCounter(int32 player_id, const FString& counter_name, float value, bool absolute) override;
    bool GetPlayerCounter(int32 player_id, const FString& counter_name, float& value) override;
    
    FDriftServerRegisteredDelegate& OnServerRegistered() override { return onServerRegistered; }
    FDriftPlayerAddedToMatchDelegate& OnPlayerAddedToMatch() override { return onPlayerAddedToMatch; }
    FDriftPlayerRemovedFromMatchDelegate& OnPlayerRemovedFromMatch() override { return onPlayerRemovedFromMatch; }
    FDriftMatchAddedDelegate& OnMatchAdded() override { return onMatchAdded; }
    FDriftMatchUpdatedDelegate& OnMatchUpdated() override { return onMatchUpdated; }

private:
    void GetRootEndpoints(TFunction<void()> onSuccess);
    void InitAuthentication(const FString& credentialType);
    void GetUserInfo();
    void RegisterClient();
    void GetPlayerEndpoints();
    void GetPlayerInfo();
    
    void AuthenticatePlayer(IDriftAuthProvider* provider);

    void AddPlayerIdentity(IDriftAuthProvider* provider, const FDriftAddPlayerIdentityProgressDelegate& progressDelegate);
    void BindUserIdentity(const FString& newIdentityName, const FDriftAddPlayerIdentityProgressDelegate& progressDelegate);

    /**
     * Associate the new identity with the current user.
     * This allows us to log in with either identity for this user in the future.
     */
    void ConnectNewIdentityToCurrentUser(const FString& newIdentityName, const FDriftAddPlayerIdentityProgressDelegate& progressDelegate);

    /**
     * Disassociate the current identity with its user, and associate it with the user tied to the new identity.
     * The new identity must have a user or this will fail.
     * Use this when recovering an account on a new or restored device where the current user has
     * been created with temporary credentials.
     * The previous user will no longer be associated with this identity and might not be recoverable unless there
     * are additional identities tied to it.
     */
    void MoveCurrentIdentityToUserOfNewIdentity(const FDriftUserInfoResponse& targetUser, const FDriftAddPlayerIdentityProgressDelegate& progressDelegate);

    void InitServerRootInfo();
    void InitServerAuthentication();
    void InitServerRegistration();
    void InitServerInfo(const FString& server_url);

    /**
     * Disconnect the player if connected, flush counters and events, and reset the internal state.
     * Will broadcast connection state changes to subscribers.
     */
    void Disconnect();

    /**
     * Reset the internal state.
     * Does not send any notifications, and should be used to get back to a clean slate in case of
     * a fatal error, or after being kicked out by the backend.
     */
    void Reset();

private:
    FDriftPlayerAuthenticatedDelegate onPlayerAuthenticated;
    FDriftConnectionStateChangedDelegate onConnectionStateChanged;
    FDriftFriendPresenceChangedDelegate onFriendPresenceChanged;
    FDriftRecievedMatchInviteDelegate onReceivedMatchInvite;
    FDriftStaticDataLoadedDelegate onStaticDataLoaded;
    FDriftStaticDataProgressDelegate onStaticDataProgress;
    FDriftPlayerStatsLoadedDelegate onPlayerStatsLoaded;
    FDriftPlayerGameStateLoadedDelegate onPlayerGameStateLoaded;
    FDriftPlayerGameStateSavedDelegate onPlayerGameStateSaved;
    FDriftGotActiveMatchesDelegate onGotActiveMatches;
    FDriftPlayerNameSetDelegate onPlayerNameSet;
    FDriftStaticRoutesInitializedDelegate onStaticRoutesInitialized;
    FDriftPlayerDisconnectedDelegate onPlayerDisconnected;
    FDriftGameVersionMismatchDelegate onGameVersionMismatch;
    FDriftUserErrorDelegate onUserError;
    FDriftServerErrorDelegate onServerError;

    FDriftServerRegisteredDelegate onServerRegistered;
    FDriftPlayerAddedToMatchDelegate onPlayerAddedToMatch;
    FDriftPlayerRemovedFromMatchDelegate onPlayerRemovedFromMatch;
    FDriftMatchAddedDelegate onMatchAdded;
    FDriftMatchUpdatedDelegate onMatchUpdated;
    
    TSharedPtr<JsonRequestManager> GetRootRequestManager();
    TSharedPtr<JsonRequestManager> GetGameRequestManager();
    void SetGameRequestManager(TSharedPtr<JsonRequestManager> manager)
    {
        authenticatedRequestManager = manager;
    }

	void TickHeartbeat(float deltaTime);
    void TickMatchInvites();
    void TickFriendUpdates(float deltaTime);

    void BeginGetFriendLeaderboard(const FString& counter_name, const TWeakPtr<FDriftLeaderboard>& leaderboard, const FDriftLeaderboardLoadedDelegate& delegate);
    void BeginGetLeaderboard(const FString& counter_name, const TWeakPtr<FDriftLeaderboard>& leaderboard, const FString& player_group, const FDriftLeaderboardLoadedDelegate& delegate);
    void GetLeaderboardImpl(const FString& counter_name, const TWeakPtr<FDriftLeaderboard>& leaderboard, const FString& player_group, const FDriftLeaderboardLoadedDelegate& delegate);

    void LoadPlayerGameStateImpl(const FString& name, const FDriftGameStateLoadedDelegate& delegate);
    void SavePlayerGameStateImpl(const FString& name, const FString& state, const FDriftGameStateSavedDelegate& delegate);

    void LoadPlayerGameStateInfos(TFunction<void(bool)> next);

    void JoinMatchQueueImpl(const FString& ref, const FString& placement, const FString& token, const FDriftJoinedMatchQueueDelegate& delegate);

    void HandleMatchQueueMessage(const FMessageQueueEntry& message);

    bool IsPreAuthenticated() const;
    bool IsPreRegistered() const;

    void CreatePlayerCounterManager();
    void CreateEventManager();
    void CreateLogForwarder();
    void CreateMessageQueue();

    void CachePlayerInfo(int32 player_id);

    void CacheFriendInfos(TFunction<void(bool)> delegate);
    void UpdateFriendOnlineInfos();

    const FDriftPlayerResponse* GetFriendInfo(int32 player_id) const;

    /**
     * Return the instance name to use for the server process
     */
    FString GetInstanceName() const;
    
    /**
     * Return the public IP passed on the command line, or the host address
     */
    FString GetPublicIP() const;

    void DefaultErrorHandler(ResponseContext& context);

    TUniquePtr<IDriftAuthProvider> GetDeviceIDCredentials(int32 index);

    FString GetApiKeyHeader() const;

    const FDriftCounterInfo* GetCounterInfo(const FString& counter_name) const;

    void ConfigurePlacement();
    void ConfigureBuildReference();

    EDriftConnectionState InternalToPublicState(DriftSessionState internalState) const;

    void BroadcastConnectionStateChange(DriftSessionState internalState);

    TUniquePtr<IDriftAuthProvider> MakeAuthProvider(const FString& credentialType);

private:
    // TODO: deprecate or consolidate with other properties
    struct CLI
    {
        FString public_ip;
        FString drift_url;
        FString server_url;
        FString port;
        FString jti;
    } cli;
    
    const FName instanceName_;
    const FString instanceDisplayName_;
    const int32 instanceIndex_;

    float heartbeatDueInSeconds_{ FLT_MAX };
    FDateTime heartbeatTimeout_{ FDateTime::MinValue() };

    DriftSessionState state_;

    TSharedPtr<JsonRequestManager> rootRequestManager_;
    TSharedPtr<JsonRequestManager> authenticatedRequestManager;
    TSharedPtr<JsonRequestManager> secondaryIdentityRequestManager_;

    FDriftEndpointsResponse driftEndpoints;

    FClientRegistrationResponse driftClient;
    FDriftPlayerResponse myPlayer;

    FString hearbeatUrl;

    TUniquePtr<FDriftCounterManager> playerCounterManager;
    TMap<int32, TUniquePtr<FDriftCounterManager>> serverCounterManagers;

    TUniquePtr<FDriftEventManager> eventManager;

    TUniquePtr<FDriftMessageQueue> messageQueue;

    TUniquePtr<FLogForwarder> logForwarder;

    bool countersLoaded = false;
    TArray<FDriftCounterInfo> counterInfos;

    bool playerGameStateInfosLoaded = false;
    TArray<FDriftPlayerGameStateInfo> playerGameStateInfos;

    bool userIdentitiesLoaded = false;
    FDriftCreatePlayerGroupResponse userIdentities;

    TArray<FString> externalFriendIDs;
    
    TMap<int32, FDriftPlayerResponse> friendInfos;
    bool shouldUpdateFriends = false;
    float updateFriendsInSeconds = 0.0f;

    FServerRegistrationResponse drift_server;

    FGetActiveMatchesResponse cached_matches;
    FMatchQueueResponse matchQueue;
    EMatchQueueState matchQueueState = EMatchQueueState::Idle;
    TArray<FMatchInvite> matchInvites;

    FGetMatchesResponseItem match_info;

    FString apiKey;
    FString projectName = TEXT("DefaultDriftProject");
    FString gameVersion = TEXT("0.0.0");
    FString gameBuild = TEXT("0");
    FString environment = TEXT("dev");
    FString buildReference;
    FString staticDataReference;
    FString defaultPlacement;

    TUniquePtr<IDriftAuthProviderFactory> deviceAuthProviderFactory;
    TSharedPtr<IDriftAuthProvider> authProvider;

    TSharedPtr<IHttpCache> httpCache_;
};


typedef TSharedPtr<FDriftBase, ESPMode::ThreadSafe> DriftBasePtr;
