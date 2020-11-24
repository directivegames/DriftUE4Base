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
    FDriftBase(const TSharedPtr<IHttpCache>& cache, const FName& instanceName, int32 instanceIndex, const FString& config);
    FDriftBase(const FDriftBase& other) = delete;
    virtual ~FDriftBase();

    // FTickableGameObject API
    void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return true; }

    TStatId GetStatId() const;

    // Generic API
	void AuthenticatePlayer() override;
	void AuthenticatePlayer(FAuthenticationSettings AuthenticationSettings) override;
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
    void InvitePlayerToMatch(int32 playerID, const FDriftJoinedMatchQueueDelegate& delegate) override;
    void JoinMatch(const FMatchInvite& invite, const FDriftJoinedMatchQueueDelegate& delegate) override;
    void AcceptMatchInvite(const FMatchInvite& invite, const FDriftJoinedMatchQueueDelegate& delegate) override;

    void AddCount(const FString& counterName, float value, bool absolute) override;
    bool GetCount(const FString& counterName, float& value) override;

    void AddAnalyticsEvent(const FString& eventName, const TArray<FAnalyticsEventAttribute>& attributes) override;
    void AddAnalyticsEvent(TUniquePtr<IDriftEvent> event) override;

    void LoadStaticData(const FString& name, const FString& ref) override;

    void LoadPlayerStats() override;

    void LoadPlayerGameState(const FString& name, const FDriftGameStateLoadedDelegate& delegate) override;
    void SavePlayerGameState(const FString& name, const FString& gameState, const FDriftGameStateSavedDelegate& delegate) override;

    void GetLeaderboard(const FString& counterName, const TSharedRef<FDriftLeaderboard>& leaderboard, const FDriftLeaderboardLoadedDelegate& delegate) override;
    void GetFriendsLeaderboard(const FString& counterName, const TSharedRef<FDriftLeaderboard>& leaderboard, const FDriftLeaderboardLoadedDelegate& delegate) override;

    void LoadFriendsList(const FDriftFriendsListLoadedDelegate& delegate) override;
    void UpdateFriendsList() override;
    bool GetFriendsList(TArray<FDriftFriend>& friends) override;
    FString GetFriendName(int32 friendID) override;
    bool IssueFriendToken(int32 PlayerID, const FDriftRequestFriendTokenDelegate& delegate) override;
    bool AcceptFriendRequestToken(const FString& token, const FDriftAcceptFriendRequestDelegate& delegate) override;
    bool RemoveFriend(int32 friendID, const FDriftRemoveFriendDelegate& delegate) override;
    void LoadPlayerAvatarUrl(const FDriftLoadPlayerAvatarUrlDelegate& delegate) override;

    bool FindPlayersByName(const FString& SearchString, const FDriftFindPlayerByNameDelegate& delegate) override;

    void FlushCounters() override;
    void FlushEvents() override;

    void Shutdown() override;

    const TMap<FString, FDateTime>& GetDeprecations() override;

    FString GetJWT() const override;
    FString GetJTI() const override;
    FString GetRootURL() const override;
    FString GetEnvironment() const override;
    FString GetGameVersion() const override;
    FString GetGameBuild() const override;
    FString GetVersionedAPIKey() const override;

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

    FDriftFriendAddedDelegate& OnFriendAdded() override { return onFriendAdded;  }
    FDriftFriendRemovedDelegate& OnFriendRemoved() override { return onFriendRemoved; }

    FDriftStaticRoutesInitializedDelegate& OnStaticRoutesInitialized() override { return onStaticRoutesInitialized; }
    FDriftPlayerDisconnectedDelegate& OnPlayerDisconnected() override { return onPlayerDisconnected; }
    FDriftGameVersionMismatchDelegate& OnGameVersionMismatch() override { return onGameVersionMismatch; }
    FDriftUserErrorDelegate& OnUserError() override { return onUserError; }
    FDriftServerErrorDelegate& OnServerError() override { return onServerError; }
    FDriftNewDeprecationDelegate OnDeprecation() override { return onDeprecation; }

    // Server API
    bool RegisterServer() override;

    void AddMatch(const FString& mapName, const FString& gameMode, int32 numTeams, int32 maxPlayers) override;
    void UpdateServer(const FString& status, const FString& reason, const FDriftServerStatusUpdatedDelegate& delegate) override;
    void UpdateMatch(const FString& status, const FString& reason, const FDriftMatchStatusUpdatedDelegate& delegate) override;
    void UpdateMatch(const FString& status, const FDriftMatchStatusUpdatedDelegate& delegate) override;
	void UpdateMatch(const FDriftUpdateMatchProperties& properties, const FDriftMatchStatusUpdatedDelegate& delegate) override;
    int32 GetMatchID() const override;
    void AddPlayerToMatch(int32 playerID, int32 teamID, const FDriftPlayerAddedDelegate& delegate) override;
    void RemovePlayerFromMatch(int32 playerID, const FDriftPlayerRemovedDelegate& delegate) override;
    void ModifyPlayerCounter(int32 playerID, const FString& counterName, float value, bool absolute) override;
    bool GetPlayerCounter(int32 playerID, const FString& counterName, float& value) override;

    FDriftServerRegisteredDelegate& OnServerRegistered() override { return onServerRegistered; }
    FDriftPlayerAddedToMatchDelegate& OnPlayerAddedToMatch() override { return onPlayerAddedToMatch; }
    FDriftPlayerRemovedFromMatchDelegate& OnPlayerRemovedFromMatch() override { return onPlayerRemovedFromMatch; }
    FDriftMatchAddedDelegate& OnMatchAdded() override { return onMatchAdded; }
    FDriftMatchUpdatedDelegate& OnMatchUpdated() override { return onMatchUpdated; }

	FDriftReceivedMessageDelegate& OnReceivedTextMessage() override { return onReceivedTextMessage; }
	FDriftReceivedMessageDelegate& OnReceivedJsonMessage() override { return onReceivedJsonMessage; }

	bool SendFriendMessage(int32 FriendId, const FString& Message) override;
	bool SendFriendMessage(int32 FriendId, class JsonValue&& Message) override;

    int32 GetInstanceIndex() const override { return instanceIndex_; }

    void SetForwardedLogLevel(ELogVerbosity::Type Level) override;

private:
    void ConfigureSettingsSection(const FString& config);

    void GetRootEndpoints(TFunction<void()> onSuccess);
    void InitAuthentication(const FAuthenticationSettings& AuthenticationSettings);
    void GetUserInfo();
    void RegisterClient();
    void GetPlayerEndpoints();
    void GetPlayerInfo();

    void AuthenticatePlayer(IDriftAuthProvider* provider);

    void AddPlayerIdentity(const TSharedPtr<IDriftAuthProvider>& provider, const FDriftAddPlayerIdentityProgressDelegate& progressDelegate);
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
    void InitServerInfo(const FString& serverUrl);

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
    FDriftFriendAddedDelegate onFriendAdded;
    FDriftFriendRemovedDelegate onFriendRemoved;
    FDriftStaticRoutesInitializedDelegate onStaticRoutesInitialized;
    FDriftPlayerDisconnectedDelegate onPlayerDisconnected;
    FDriftGameVersionMismatchDelegate onGameVersionMismatch;
    FDriftUserErrorDelegate onUserError;
    FDriftServerErrorDelegate onServerError;
    FDriftNewDeprecationDelegate onDeprecation;

    FDriftServerRegisteredDelegate onServerRegistered;
    FDriftPlayerAddedToMatchDelegate onPlayerAddedToMatch;
    FDriftPlayerRemovedFromMatchDelegate onPlayerRemovedFromMatch;
    FDriftMatchAddedDelegate onMatchAdded;
    FDriftMatchUpdatedDelegate onMatchUpdated;

	FDriftReceivedMessageDelegate onReceivedTextMessage;
	FDriftReceivedMessageDelegate onReceivedJsonMessage;

    TSharedPtr<JsonRequestManager> GetRootRequestManager() const;
    TSharedPtr<JsonRequestManager> GetGameRequestManager() const;
    void SetGameRequestManager(TSharedPtr<JsonRequestManager> manager)
    {
        authenticatedRequestManager = manager;
    }

	void TickHeartbeat(float deltaTime);
    void TickMatchInvites();
    void TickFriendUpdates(float deltaTime);

    void BeginGetFriendLeaderboard(const FString& counterName, const TWeakPtr<FDriftLeaderboard>& leaderboard, const FDriftLeaderboardLoadedDelegate& delegate);
    void BeginGetLeaderboard(const FString& counterName, const TWeakPtr<FDriftLeaderboard>& leaderboard, const FString& playerGroup, const FDriftLeaderboardLoadedDelegate& delegate);
    void GetLeaderboardImpl(const FString& counterName, const TWeakPtr<FDriftLeaderboard>& leaderboard, const FString& playerGroup, const FDriftLeaderboardLoadedDelegate& delegate);

    void LoadPlayerGameStateImpl(const FString& name, const FDriftGameStateLoadedDelegate& delegate);
    void SavePlayerGameStateImpl(const FString& name, const FString& state, const FDriftGameStateSavedDelegate& delegate);

    void LoadPlayerGameStateInfos(TFunction<void(bool)> next);

    void JoinMatchQueueImpl(const FString& ref, const FString& placement, const FString& token, const FDriftJoinedMatchQueueDelegate& delegate);

    void HandleMatchQueueMessage(const FMessageQueueEntry& message);
    void HandleFriendEventMessage(const FMessageQueueEntry& message);
	void HandleFriendMessage(const FMessageQueueEntry& message);

    bool IsPreAuthenticated() const;
    bool IsPreRegistered() const;

    void CreatePlayerCounterManager();
    void CreateEventManager();
    void CreateLogForwarder();
    void CreateMessageQueue();

    void CachePlayerInfo(int32 playerID);

    void LoadDriftFriends(const FDriftFriendsListLoadedDelegate& delegate);
    void MakeFriendsGroup(const FDriftFriendsListLoadedDelegate& delegate);
    void CacheFriendInfos(const TFunction<void(bool)>& delegate);
    void UpdateFriendOnlineInfos();

    const FDriftPlayerResponse* GetFriendInfo(int32 playerID) const;

    /**
     * Return the instance name to use for the server process
     */
    FString GetInstanceName() const;

    /**
     * Return the public IP passed on the command line, or the host address
     */
    FString GetPublicIP() const;

    void DefaultErrorHandler(ResponseContext& context);
    void DriftDeprecationMessageHandler(const FString& deprecations);
    void ParseDeprecation(const FString& deprecation);

    TUniquePtr<IDriftAuthProvider> GetDeviceIDCredentials(int32 index);

    FString GetApiKeyHeader() const;

    const FDriftCounterInfo* GetCounterInfo(const FString& counterName) const;

    void ConfigurePlacement();
    void ConfigureBuildReference();

    static EDriftConnectionState InternalToPublicState(DriftSessionState internalState);

    void BroadcastConnectionStateChange(DriftSessionState internalState) const;

    static TUniquePtr<IDriftAuthProvider> MakeAuthProvider(const FString& credentialType);

    bool IsRunningAsServer() const;

    const FString& GetProjectName();
    const FGuid& GetAppGuid();
    IDriftAuthProviderFactory* GetDeviceAuthProviderFactory();
	IDriftAuthProviderFactory* GetUserPassAuthProviderFactory(const FString& Username, const FString& Password, bool bAllowAutomaticAccountCreation);

	bool DoSendFriendMessage(int32 FriendId, JsonValue&& MessagePayload);

private:
    FString settingsSection_;

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

    TMap<int32, FDriftFriendResponse> driftFriends;
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
	FString versionedApiKey;
    FGuid appGuid_;
    FString projectName_ = TEXT("DefaultDriftProject");
    FString gameVersion = TEXT("0.0.0");
    FString gameBuild = TEXT("0");
    FString environment = TEXT("dev");
    FString buildReference;
    FString staticDataReference;
    FString defaultPlacement;

    TUniquePtr<IDriftAuthProviderFactory> deviceAuthProviderFactory_;
    TUniquePtr<IDriftAuthProviderFactory> userPassAuthProviderFactory_;
    TSharedPtr<IDriftAuthProvider> authProvider;

    TSharedPtr<IHttpCache> httpCache_;

    TMap<FString, FDateTime> deprecations_;
    FString previousDeprecationHeader_;

    bool ignoreCommandLineArguments_ = false;

	FString serverJTI_;
	FString serverJWT_;
};


typedef TSharedPtr<FDriftBase, ESPMode::ThreadSafe> DriftBasePtr;
