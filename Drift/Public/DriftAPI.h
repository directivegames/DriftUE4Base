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

#include "IDriftPartyManager.h"
#include "IDriftMatchmaker.h"
#include "IDriftLobbyManager.h"
#include "IDriftMatchPlacementManager.h"
#include "IDriftSandboxManager.h"
#include "JsonValueWrapper.h"

#include "DriftAPI.generated.h"

struct FRichPresence;
class IDriftMessageQueue;

/**
 * Fired when server registration has completed.
 * bool - success
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FDriftServerRegisteredDelegate, bool);

/**
 * Fired when AddMatch() finishes
 * bool - success
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FDriftMatchAddedDelegate, bool);

/**
 * Fired when UpdateMatch() finishes
 * bool - success
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FDriftMatchUpdatedDelegate, bool);

/**
 * Fired when AddPlayerToMatch() finishes. Use this when you're not the
 * instigator.
 * bool - success
 * int32 - player id
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftPlayerAddedToMatchDelegate, bool, int32);

/**
 * Fired when RemovePlayerFromMatch() finishes. Use this when you're not the
 * instigator.
 * bool - success
 * int32 - player id
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftPlayerRemovedFromMatchDelegate, bool, int32);

/**
 * Fired when UpdatePlayerInMatch() finishes. Use this when you're not the
 * instigator.
 * bool - success
 * int32 - player id
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftPlayerUpdatedInMatchDelegate, bool, int32);

/**
 * Fired to notify the original caller when AddPlayerToMatch() finishes.
 * bool - success
 */
DECLARE_DELEGATE_OneParam(FDriftPlayerAddedDelegate, bool);

/**
 * Fired to notify the original caller when RemovePlayerFromMatch() finishes.
 * bool - success
 */
DECLARE_DELEGATE_OneParam(FDriftPlayerRemovedDelegate, bool);

/**
 * Fired to notify the original caller when UpdatePlayerInMatch() finishes.
 * bool - success
 */
DECLARE_DELEGATE_OneParam(FDriftPlayerUpdatedDelegate, bool);

DECLARE_DELEGATE_OneParam(FDriftServerStatusUpdatedDelegate, bool);
DECLARE_DELEGATE_OneParam(FDriftMatchStatusUpdatedDelegate, bool);

struct FAnalyticsEventAttribute;
class IDriftEvent;


struct FDriftUpdateMatchProperties
{
    TOptional<FString> gameMode;
    TOptional<FString> mapName;
    TOptional<FString> status;
    TOptional<int32> maxPlayers;

   	TOptional<JsonValue> details;
   	TOptional<JsonValue> match_statistics;

    /**
     * uniqueKey enforces uniqueness of running matches by not allowing Drift to accept two matches with the same
     * unique key. What the unique key is, and what it means to an individual product is left to the product to define
     */
    TOptional<FString> uniqueKey;
};

struct FDriftUpdateMatchPlayerProperties
{
    TOptional<FString> status;
    TOptional<int32> team_id;
    TOptional<JsonValue> details;
    TOptional<JsonValue> statistics;
};

struct FDriftMatchTeam
{
    int32 TeamId = 0;
    int32 MatchId = 0;

    FDateTime CreateDate;

    FString TeamName;

    JsonValue Details;
    JsonValue Statistics;

    FString Url;
};

class IDriftServerAPI
{
public:
    /**
     * Server Specific API
     */

    /**
     * Register this server instance with Drift
     * Normally the server manager has already authenticated and is passing the JTI
     * on the command line, via "-jti=xxx" so this call will proceed to start the heartbeat
     * and fetch session information, before notifying the game.
     */
    virtual bool RegisterServer() = 0;

    /**
     * For a match to show up in Drift match making, it needs to be registered.
     */
    virtual void AddMatch(const FString& mapName, const FString& gameMode, int32 numTeams, int32 maxPlayers) = 0;
    virtual void AddMatch(const FString& mapName, const FString& gameMode, TArray<FString> teamNames, int32 maxPlayers) = 0;

    /**
    * Update a server to set it's status for the match maker.
    */
    virtual void UpdateServer(const FString& status, const FString& reason, const FDriftServerStatusUpdatedDelegate& delegate) = 0;

    /**
     * Update a match to set it's status for the match maker. A status of "completed" means the match has ended.
     */
    virtual void UpdateMatch(const FString& status, const FString& reason, const FDriftMatchStatusUpdatedDelegate& delegate) = 0;
    virtual void UpdateMatch(const FString& status, const FDriftMatchStatusUpdatedDelegate& delegate) = 0;
    virtual void UpdateMatch(const FDriftUpdateMatchProperties& properties, const FDriftMatchStatusUpdatedDelegate& delegate) = 0;

    /**
     * Get the match ID if currently hosting a match, or 0.
     */
    virtual int32 GetMatchID() const = 0;

    /**
     * Register a player with the current match. This lets the backend know the player has
     * successfully connected to the match.
     */
    virtual void AddPlayerToMatch(int32 playerID, int32 teamID, const FDriftPlayerAddedDelegate& delegate) = 0;

    /**
     * Remove a player from the current match. This should be done if the player disconnects,
     * but the match isn't ending. When the match is set to "completed", players are automatically removed.
     */
    virtual void RemovePlayerFromMatch(int32 playerID, const FDriftPlayerRemovedDelegate& delegate) = 0;

    /**
     * Update a player in the current match.
     */
     virtual void UpdatePlayerInMatch(int32 playerID, const FDriftUpdateMatchPlayerProperties& properties, const FDriftPlayerUpdatedDelegate& delegate) = 0;

    /**
     * Modify a player's counter. Counters are automatically loaded for each player
     * as they are added to the match.
     */
    virtual void ModifyPlayerCounter(int32 playerID, const FString& counterName, float value, bool absolute) = 0;

    /**
     * Get a player's counter. Counters are automatically loaded for each player
     * as they are added to the match.
     */
    virtual bool GetPlayerCounter(int32 playerID, const FString& counterName, float& value) = 0;

	virtual TArray<FDriftMatchTeam> GetMatchTeams() const = 0;
    virtual TOptional<FDriftMatchTeam> GetMatchTeam(const FString& TeamName) const = 0;

    /**
     * Server Specific Notifications
     */

    virtual FDriftServerRegisteredDelegate& OnServerRegistered() = 0;

    virtual FDriftMatchAddedDelegate& OnMatchAdded() = 0;
    virtual FDriftMatchUpdatedDelegate& OnMatchUpdated() = 0;

    virtual FDriftPlayerAddedToMatchDelegate& OnPlayerAddedToMatch() = 0;
    virtual FDriftPlayerRemovedFromMatchDelegate& OnPlayerRemovedFromMatch() = 0;

    virtual ~IDriftServerAPI() {}
};


struct FActiveMatch
{
    int32 match_id{0};
    int32 max_players{0};
    int32 num_players{0};

    FDateTime create_date;
    FString game_mode;
    FString map_name;
    FString match_status;
    FString server_status;
    FString ue4_connection_url;
    FString version;
};


struct FMatchQueueMatch
{
    int32 match_id;

    FDateTime create_date;
    FString ue4_connection_url;
};


struct FMatchesSearch
{
    TArray<FActiveMatch> matches;
    TOptional<FString> ref_filter;
    TOptional<TArray<int32>> match_id_filter;
};


struct FMatchQueueStatus
{
    FName status;
    FMatchQueueMatch match;
};


struct FMatchInvite
{
    int32 playerID = 0;
    FString token;
    FDateTime sent;
    FDateTime expires;

    FMatchInvite() = default;
    FMatchInvite(const FMatchInvite& other) = default;

    FMatchInvite(int32 _playerID, const FString& _token, const FDateTime& _sent, const FDateTime& _expires)
        : playerID{_playerID}
          , token{_token}
          , sent{_sent}
          , expires{_expires}
    {
        // Intentionally empty
    }

    bool operator==(const FMatchInvite& other) const
    {
        return (playerID == other.playerID) && (token == other.token);
    }
};


UENUM(BlueprintType)
enum class EAuthenticationResult : uint8
{
    Success,
    Error_Config,
    Error_Forbidden,
    Error_NoOnlineSubsystemCredentials,
    Error_Failed,
    Error_InvalidCredentials,
};


struct FPlayerAuthenticatedInfo
{
    int32 playerId;
    FString playerName;
    EAuthenticationResult result;
    FString error;

    FPlayerAuthenticatedInfo(EAuthenticationResult _result, const FString& _error)
        : result{_result}
          , error{_error}
    {
    }

    FPlayerAuthenticatedInfo(int32 _playerId, const FString& _playerName)
        : playerId{_playerId}
          , playerName{_playerName}
          , result{EAuthenticationResult::Success}
    {
    }
};

struct FDriftLeaderboardEntry
{
    FString player_name;
    int32 player_id;
    float value;
    int32 position;
};


enum class ELeaderboardState : uint8
{
    Failed,
    Loading,
    Ready
};


struct FDriftLeaderboard
{
    FString name;
    ELeaderboardState state;
    TArray<FDriftLeaderboardEntry> rows;
};


UENUM(BlueprintType)
enum class EDriftPresence : uint8
{
    Unknown,
    Offline,
    Online
};


UENUM(BlueprintType)
enum class EDriftFriendType : uint8
{
    NotFriend,
    Drift,
    External
};


struct FDriftFriend
{
    int32 playerID;
    FString name;
    EDriftPresence presence;
    EDriftFriendType type;
};


UENUM(BlueprintType)
enum class ELoadPlayerGameStateResult : uint8
{
    Success,
    Error_InvalidState,
    Error_NotFound,
    Error_Failed,
};


UENUM(BlueprintType)
enum class EMatchQueueState : uint8
{
    Idle UMETA(DisplayName = "Not In Queue"),
    Joining UMETA(DisplayName = "Joining"),
    Queued UMETA(DisplayName = "In Queue"),
    Updating UMETA(DisplayName = "Updating"),
    Matched UMETA(DisplayName = "Matched"),
    Leaving UMETA(DisplayName = "Leaving"),
};


UENUM(BlueprintType)
enum class EDriftConnectionState : uint8
{
    Disconnected UMETA(DisplayName = "Not connected"),
    Authenticating UMETA(DisplayName = "Authenticating"),
    Connected UMETA(DisplayName = "Connected"),
    Timedout UMETA(DisplayName = "Timed Out"),
    Usurped UMETA(DisplayName = "Usurped"),
    Disconnecting UMETA(DisplayName = "Disconnecting"),
};


enum class EPlayerIdentityAssignOption : uint8
{
    DoNotAssignIdentityToUser,
    AssignIdentityToExistingUser
};


enum class EPlayerIdentityOverrideOption : uint8
{
    DoNotOverrideExistingUserAssociation,
    AssignIdentityToNewUser
};


enum class EAddPlayerIdentityStatus : uint8
{
    Unknown,
    Success_NewIdentityAddedToExistingUser,
    Success_NoChange,
    Success_OldIdentityMovedToNewUser,
    Progress_IdentityCanBeAssociatedWithUser,
    Progress_IdentityAssociatedWithOtherUser,
    Error_FailedToAquireCredentials,
    Error_FailedToAuthenticate,
    Error_FailedToReAssignOldIdentity,
    Error_UserAlreadyBoundToSameIdentityType,
    Error_Failed
};


DECLARE_DELEGATE_OneParam(FDriftPlayerIdentityAssignContinuationDelegate, EPlayerIdentityAssignOption);
DECLARE_DELEGATE_OneParam(FDriftPlayerIdentityOverrideContinuationDelegate, EPlayerIdentityOverrideOption);


struct FDriftAddPlayerIdentityProgress
{
    FDriftAddPlayerIdentityProgress()
        : FDriftAddPlayerIdentityProgress(EAddPlayerIdentityStatus::Unknown)
    {
    }

    FDriftAddPlayerIdentityProgress(EAddPlayerIdentityStatus inStatus)
        : status{inStatus}
    {
    }

    EAddPlayerIdentityStatus status;
    FString localUserPlayerName;
    FString newIdentityName;
    FString newIdentityUserPlayerName;
    FDriftPlayerIdentityAssignContinuationDelegate assignDelegate;
    FDriftPlayerIdentityOverrideContinuationDelegate overrideDelegate;
};

struct FDriftMatchPlayer
{
    int32 Id = 0;
    int32 MatchId = 0;
    int32 PlayerId = 0;
    int32 TeamId = 0;

    FDateTime CreateDate;
    FDateTime JoinDate;
    FDateTime LeaveDate;

    FString PlayerName;
    FString Status;
    int32 NumJoins = 0;
    int32 Seconds = 0;

    JsonValue Details;
    JsonValue Statistics;

    FString MatchPlayerUrl;
    FString PlayerUrl;
};

struct FDriftMatch
{
    int32 MatchId = 0;
    int32 ServerId = 0;

    FDateTime CreateDate;
    FDateTime StartDate;
    FDateTime EndDate;

    FString GameMode;
    FString MapName;
    FString Status;
    int32 NumPlayers = 0;
    int32 MaxPlayers = 0;

    JsonValue Details;
    JsonValue Statistics;

    TOptional<TArray<FDriftMatchPlayer>> Players;
    TOptional<TArray<FDriftMatchTeam>> Teams;

    FString Url;
    FString MatchPlayersUrl;
    FString TeamsUrl;
};

struct FDriftMatchesResult
{
    TArray<FDriftMatch> Matches;
    int32 TotalMatches = 0;
    int32 CurrentPage = 0;
    int32 Pages = 0;
    int32 MatchesPerPage = 0;
};

struct FRichPresenceResult: FJsonSerializable
{
    BEGIN_JSON_SERIALIZER;
        JSON_SERIALIZE("player_id", player_id);
        JSON_SERIALIZE("is_online", is_online);
        JSON_SERIALIZE("is_in_game", is_in_game);
        JSON_SERIALIZE("map_name", map_name);
        JSON_SERIALIZE("game_mode", game_mode);
    END_JSON_SERIALIZER;

    int32 player_id;
    bool is_online;
    bool is_in_game;
    FString map_name;
    FString game_mode;

    bool Serialize(class SerializationContext& context);
};

struct FGetDriftMatchesParameters
{
    int32 PageNumber = 1;
    int32 MatchesPerPage = 20;
    bool bIncludePlayers = false;

    TOptional<int32> PlayerId;
    TOptional<FString> GameMode;
    TOptional<FString> MapName;
    TOptional<TMap<FString, FString>> StatisticsFilter;
    TOptional<TMap<FString, FString>> DetailsFilter;
};

class JsonValue;
struct FDriftPlayerResponse;

/**
 * Array item response from GET endpoints.user_identities
 */
struct FDriftUserIdentity
{
    int32 player_id = 0;
    FString identity_name;
    FString player_name;
    FString player_url;

    bool Serialize(class SerializationContext& context);
};


enum class EMessageType : uint8
{
    Text,
    Json,
};

struct FDriftMessage
{
    // the type of the message
    EMessageType messageType = EMessageType::Text;

    // the id of the player who sends the message
    int32 senderId = 0;

    // increasing message number, might get reset after a period
    int32 messageNumber = 0;

    // unique id of the message
    FString messageId;

    // when the message was sent
    FDateTime sendTime;

    // when the message expires
    FDateTime expireTime;

    // for text message this is the text itself
    // for json message this is the json object string
    FString messageBody;
};

struct FDriftFriendRequest
{
    int32 id;
    FDateTime create_date;
    FDateTime expiry_date;

    int32 issued_by_player_id;
    FString issued_by_player_url;
    FString issued_by_player_name;

    int32 issued_to_player_id;
    FString issued_to_player_url;
    FString issued_to_player_name;

    FString accept_url;
    FString token;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftPlayerAuthenticatedDelegate, bool, const FPlayerAuthenticatedInfo&);
DECLARE_MULTICAST_DELEGATE_OneParam(FDriftConnectionStateChangedDelegate, EDriftConnectionState);
DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftStaticDataLoadedDelegate, bool, const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftStaticDataProgressDelegate, const FString&, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FDriftGotActiveMatchesDelegate, bool);
DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftPlayerNameSetDelegate, bool, const FString&);
DECLARE_MULTICAST_DELEGATE(FDriftStaticRoutesInitializedDelegate);
DECLARE_MULTICAST_DELEGATE_OneParam(FDriftPlayerStatsLoadedDelegate, bool);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FDriftPlayerGameStateLoadedDelegate, ELoadPlayerGameStateResult, const FString&, const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftPlayerGameStateSavedDelegate, bool, const FString&);
DECLARE_DELEGATE_TwoParams(FDriftLeaderboardLoadedDelegate, bool, const FString&);
DECLARE_DELEGATE_OneParam(FDriftFriendsListLoadedDelegate, bool);

DECLARE_DELEGATE_ThreeParams(FDriftIssueFriendTokenDelegate, bool /* bSuccess */, const FString& /* Token */, const FString& /* Error */);
DECLARE_DELEGATE_ThreeParams(FDriftAcceptFriendRequestDelegate, bool /* bSuccess */, int32 /* Friend Id */, const FString& /* Error */);
DECLARE_DELEGATE_OneParam(FDriftDeclineFriendRequestDelegate, bool);
DECLARE_DELEGATE_TwoParams(FDriftGetFriendRequestsDelegate, bool, const TArray<FDriftFriendRequest>&);
DECLARE_DELEGATE_TwoParams(FDriftRemoveFriendDelegate, bool, int32);
DECLARE_DELEGATE_TwoParams(FDriftFindPlayerByNameDelegate, bool, const TArray<FDriftFriend>&);

DECLARE_DELEGATE_TwoParams(FDriftGetFriendRichPresenceDelegate, bool, const FRichPresenceResult);
DECLARE_DELEGATE_OneParam(FDriftGetFriendsRichPresenceDelegate, bool);

DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftFriendPresenceChangedDelegate, int32, EDriftPresence);

DECLARE_DELEGATE_OneParam(FDriftAddPlayerIdentityProgressDelegate, const FDriftAddPlayerIdentityProgress&);

DECLARE_DELEGATE_TwoParams(FDriftGetMatchesDelegate, bool /* bSuccess */, const FDriftMatchesResult& /* Matches */);

DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftGameVersionMismatchDelegate, const FString& /* message */, const FString& /* upgrade_url */);

DECLARE_DELEGATE_ThreeParams(FDriftGameStateLoadedDelegate, ELoadPlayerGameStateResult, const FString&, const FString&);
DECLARE_DELEGATE_TwoParams(FDriftGameStateSavedDelegate, bool, const FString&);

DECLARE_MULTICAST_DELEGATE(FDriftPlayerDisconnectedDelegate)
DECLARE_MULTICAST_DELEGATE(FDriftUserErrorDelegate);
DECLARE_MULTICAST_DELEGATE(FDriftServerErrorDelegate);

DECLARE_DELEGATE_TwoParams(FDriftJoinedMatchQueueDelegate, bool, const FMatchQueueStatus&);
DECLARE_DELEGATE_OneParam(FDriftLeftMatchQueueDelegate, bool);
DECLARE_DELEGATE_TwoParams(FDriftPolledMatchQueueDelegate, bool, const FMatchQueueStatus&);

DECLARE_MULTICAST_DELEGATE_OneParam(FDriftRecievedMatchInviteDelegate, const FMatchInvite&);

DECLARE_MULTICAST_DELEGATE_OneParam(FDriftFriendAddedDelegate, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FDriftFriendRemovedDelegate, int32);
DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftFriendRequestReceivedDelegate, int32, const FString&);

DECLARE_DELEGATE_OneParam(FDriftLoadPlayerAvatarUrlDelegate, const FString&);

DECLARE_DELEGATE_TwoParams(FDriftGetUserIdentitiesDelegate, bool, const TArray<FDriftUserIdentity>&);

DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftNewDeprecationDelegate, const FString&, const FDateTime&);

DECLARE_MULTICAST_DELEGATE_OneParam(FDriftReceivedMessageDelegate, const FDriftMessage& /*Message*/);

DECLARE_DELEGATE_OneParam(FDriftFetchClientConfigsComplete, bool /*bSuccess*/);


struct FAuthenticationSettings
{
    FAuthenticationSettings()
        : FAuthenticationSettings(true)
    {
    }

    FAuthenticationSettings(bool bAutoCreateAccount)
        : bAutoCreateAccount{bAutoCreateAccount}
    {
    }

    FAuthenticationSettings(const FString& CredentialsType, bool bAutoCreateAccount)
        : CredentialsType{CredentialsType}
          , bAutoCreateAccount{bAutoCreateAccount}
    {
    }

    FAuthenticationSettings(const FString& Username, const FString& Password, bool bAutoCreateAccount)
        : CredentialsType{TEXT("user+pass")}
          , Username{Username}
          , Password{Password}
          , bAutoCreateAccount{bAutoCreateAccount}
    {
    }

    FString CredentialsType;
    FString Username;
    FString Password;
    bool bAutoCreateAccount;
};

struct FDriftFriendTokenProperties
{
    TOptional<FString> TokenFormat;
    TOptional<int32> WordlistNumberOfWords;
    TOptional<int32> ExpirationTimeInSeconds;
};

class IDriftAPI : public IDriftServerAPI
{
public:
    /**
     * Client Specific API
     */

    /**
    * Authenticate a player using credential type defined in config
    * Fires OnGameVersionMismatch if the game version is invalid
    * Fires OnPlayerAuthenticated() when finished.
    */
    virtual void AuthenticatePlayer() = 0;
    virtual void AuthenticatePlayer(FAuthenticationSettings AuthenticationSettings) = 0;

    /**
     * Get connection state
     */
    virtual EDriftConnectionState GetConnectionState() const = 0;

    /**
     * Return the currently authenticated player's name, or an empty string if not authenticated.
     */
    virtual FString GetPlayerName() = 0;

    /**
     * Return the currently authenticated player's ID, or 0 if not authenticated.
     */
    virtual int32 GetPlayerID() = 0;

    /**
    * Return the currently authenticated player's ID, or 0 if not authenticated.
    */
    virtual FString GetPlayerUUID() = 0;

    /**
     * Set the currently authenticated player's name.
     * Fires OnPlayerNameSet() when finished.
     */
    virtual void SetPlayerName(const FString& name) = 0;

    /**
     * Return the name of the current auth provider
     */
    virtual FString GetAuthProviderName() const = 0;

    /**
     * Bind the identity from a secondary auth provider to the currently logged in user.
     */
    virtual void AddPlayerIdentity(const FString& authProvider, const FDriftAddPlayerIdentityProgressDelegate& progressDelegate) = 0;

    /**
     * Return a paginated result of matches
     */
    virtual void GetMatches(const FGetDriftMatchesParameters& Parameters, const FDriftGetMatchesDelegate& Delegate) = 0;

    /**
     * Return a list of active matches, available for joining
     */
    virtual void GetActiveMatches(const TSharedRef<FMatchesSearch>& search) = 0;

    /**
     * Join the match making queue
     */
    virtual void JoinMatchQueue(const FDriftJoinedMatchQueueDelegate& delegate) = 0;

    /**
     * Leave the match making queue
     * Trying to leave a queue with the status "matched" is not allowed
     */
    virtual void LeaveMatchQueue(const FDriftLeftMatchQueueDelegate& delegate) = 0;

    /**
     * Read the latest status of the match making queue
     */
    virtual void PollMatchQueue(const FDriftPolledMatchQueueDelegate& delegate) = 0;

    /**
     * Reset the state of the match making queue
     */
    virtual void ResetMatchQueue() = 0;

    /**
     * Get the current state of the match making queue
     */
    virtual EMatchQueueState GetMatchQueueState() const = 0;

    /**
     * Invite a player to a new match queue
     */
    virtual void InvitePlayerToMatch(int32 playerId, const FDriftJoinedMatchQueueDelegate& delegate) = 0;

    /**
     * Join a match in an invite
     */
    virtual void JoinMatch(const FMatchInvite& invite, const FDriftJoinedMatchQueueDelegate& delegate) = 0;

    /**
     * Accept match queue invite
     */
    virtual void AcceptMatchInvite(const FMatchInvite& invite, const FDriftJoinedMatchQueueDelegate& delegate) = 0;

    /**
     * Update a counter for the currently authenticated player.
     */
    virtual void AddCount(const FString& counterName, float value, bool absolute) = 0;

    /**
     * Return the value of a counter for the currently authenticated player.
     */
    virtual bool GetCount(const FString& counterName, float& value) = 0;

    /**
     * Post an event for the metrics system
     */
    virtual void AddAnalyticsEvent(const FString& eventName, const TArray<FAnalyticsEventAttribute>& attributes) = 0;

    /**
     * Post an event for the metrics system
     */
    virtual void AddAnalyticsEvent(TUniquePtr<IDriftEvent> event) = 0;

    /**
     * Load static data from a CDN. Requires an authenticated player.
     * Fires OnStaticDataProgress() to report progress.
     * Fires OnStaticDataLoaded() when finished.
     */
    virtual void LoadStaticData(const FString& name, const FString& ref) = 0;

    /**
     * Cache the currently authenticated player's stats.
     * Needs to be done before AddCount() and GetCount() will work.
     * Fires OnPlayerStatsLoaded() when finished.
     */
    virtual void LoadPlayerStats() = 0;

    /**
     * Loads the authenticated player's named game state.
     * Fires delegate when finished.
     */
    virtual void LoadPlayerGameState(const FString& name, const FDriftGameStateLoadedDelegate& delegate) = 0;

    /**
     * Loads the player's named game state.
     * Fires delegate when finished.
     */
    virtual void LoadPlayerGameState(int32 playerId, const FString& name, const FDriftGameStateLoadedDelegate& delegate) = 0;

    /**
     * Saves the authenticated player's named game state.
     * Fires delegate when finished.
     */
    virtual void SavePlayerGameState(const FString& name, const FString& gameState, const FDriftGameStateSavedDelegate& delegate) = 0;

    /**
     * Saves the player's named game state.
     * Fires delegate when finished.
     */
    virtual void SavePlayerGameState(int32 playerId, const FString& name, const FString& gameState, const FDriftGameStateSavedDelegate& delegate) = 0;

    /**
     * Get the global top leaderboard for counterName. Requires an authenticated player.
     * Returns data in the passed in leaderboard struct
     * Fires delegate when finished
     */
    virtual void GetLeaderboard(const FString& counterName, const TSharedRef<FDriftLeaderboard>& leaderboard, const FDriftLeaderboardLoadedDelegate& delegate) = 0;

    /**
     * Get the top leaderboard for counterName filtered on the authenticated player's friends.
     * Returns data in the passed in leaderboard struct
     * Fires delegate when finished
     */
    virtual void GetFriendsLeaderboard(const FString& counterName, const TSharedRef<FDriftLeaderboard>& leaderboard, const FDriftLeaderboardLoadedDelegate& delegate) = 0;

    /**
     * Read the friends list
     * Fires delegate when finished
     */
    virtual void LoadFriendsList(const FDriftFriendsListLoadedDelegate& delegate) = 0;

    /**
     * Request an update of the friends list. Updates are scheduled internally, so it's safe to call this at any frequency.
     * Events will be sent on completion, if anything changes.
     */
    virtual void UpdateFriendsList() = 0;

    /**
     * Get the list of friends
     */
    virtual bool GetFriendsList(TArray<FDriftFriend>& friends) = 0;

    /**
     * Get the name of a friend, if it has been cached by LoadFriendsList()
     */
    virtual FString GetFriendName(int32 friendID) = 0;

    /**
     * Gets the Rich Presence information for a specific player. The player must be a friend of the local player.
     * After doing a one-time request, you should listen for updates via the 'richpresence' message queue instead.
     */
    virtual void CacheFriendRichPresence(int32 FriendId, const FDriftGetFriendRichPresenceDelegate& Delegate) = 0;

    /**
     * Gets and caches Rich Presence information for the friends in your friend list.
     */
    virtual void CacheFriendsRichPresence(const FDriftGetFriendsRichPresenceDelegate& Delegate) = 0;

    /**
     * Gets rich presence information sync. Requires to be pre-cached.
     */
    virtual FRichPresenceResult GetRichPresence(int32 PlayerID) const = 0;

    /**
     * Sets the rich presence information for a specific player.
     */
    virtual void SetRichPresence(int32 PlayerID, const FRichPresenceResult& Presence) = 0;

    /**
     * Returns whether rich presence information is available for a specific player.
     */
    virtual const bool HasRichPresence(int32 PlayerID) const = 0;

    /**
     * Issue a friend invite token to 'PlayerID'
     * If 'playerID' is > 0, a message will be sent to that player with the token, otherwise no message is sent and
     * the token will be valid for any player who accepts it. In that case, the token must be sent to a friend via external means
     */
    virtual bool IssueFriendToken(int32 PlayerID, FDriftFriendTokenProperties TokenProperties, const FDriftIssueFriendTokenDelegate& delegate) = 0;

    /**
     * Accept a friend request via an external token
     */
    virtual bool AcceptFriendRequestToken(const FString& token, const FDriftAcceptFriendRequestDelegate& delegate) = 0;

    /**
     * Reject a previously received friend request.
     */
    virtual bool DeclineFriendRequest(int32 RequestId, FDriftDeclineFriendRequestDelegate& delegate) = 0;

    /**
     * Get Friend Requests directed at the current player
     */
    virtual bool GetFriendRequests(const FDriftGetFriendRequestsDelegate& Delegate) = 0;

    /**
     * Get Friend Requests issued by the current player
     */
    virtual bool GetSentFriendInvites(const FDriftGetFriendRequestsDelegate& Delegate) = 0;

    /**
    * Remove a friendship. This will mutually remove the player's from each other's friends lists.
    * Only supported for friends managed through Drift, i.e. with Type == EDriftFriendType::Drift
    */
    virtual bool RemoveFriend(int32 friendID, const FDriftRemoveFriendDelegate& delegate) = 0;

    /**
     * Searches for players by player_name (not username). If searchString contains a '*' for a wildcard search, the
     * search will be case-insensitive.
     */
    virtual bool FindPlayersByName(const FString& searchString, const FDriftFindPlayerByNameDelegate& delegate) = 0;

    /**
     * Load the avatar url of the currently logged in player
     * Fires delegate when finished
    */
    virtual void LoadPlayerAvatarUrl(const FDriftLoadPlayerAvatarUrlDelegate& delegate) = 0;

    /**
     * Get user-identities that match playerId
     * Fires delegate when finished.
    */
    virtual void GetUserIdentitiesByPlayerId(int32 PlayerId, const FDriftGetUserIdentitiesDelegate& delegate) = 0;

    /**
     * Get user-identities that match any name in names array
     * Fires delegate when finished.
    */
    virtual void GetUserIdentitiesByNames(const TArray<FString>& namesArray, const FDriftGetUserIdentitiesDelegate& delegate) = 0;

    /**
     * Get user-identities that match name
     * Fires delegate when finished.
    */
    virtual void GetUserIdentitiesByName(const FString& name, const FDriftGetUserIdentitiesDelegate& delegate) = 0;

    /* Gets the cached value of a client config from drift */
    virtual FString GetDriftClientConfigValue(const FString& ConfigKey) = 0;

    /* Fetch client config values from the client and update the cache*/
    virtual void FetchDriftClientConfigs(const FDriftFetchClientConfigsComplete& InDelegate) = 0;

    /**
     * Flush all counters. Requires at least one tick to actually flush.
     * This is normally called automatically on a timer. Only use it when you want to prepare for shutdown,
     * or before logging out the current player.
     */
    virtual void FlushCounters() = 0;

    /**
     * Flush all events. Requires at least one tick to actually flush.
     * This is normally called automatically on a timer. Only use it when you want to prepare for shutdown,
     * or before logging out the current player.
     */
    virtual void FlushEvents() = 0;

    /**
     * Shuts down the connection and cleans up any outstanding transactions
     */
    virtual void Shutdown() = 0;

    /**
     * Get all known feature deprecation dates
     */
    virtual const TMap<FString, FDateTime>& GetDeprecations() = 0;

    /**
     * Fired when the player finishes authenticating.
     */
    virtual FDriftPlayerAuthenticatedDelegate& OnPlayerAuthenticated() = 0;
    /**
     * Fired when the player's connection state changes.
     */
    virtual FDriftConnectionStateChangedDelegate& OnConnectionStateChanged() = 0;
    /**
     * Fired when a friend's presence changes
     */
    virtual FDriftFriendPresenceChangedDelegate& OnFriendPresenceChanged() = 0;
    /**
     * Fired when a match invite is received.
     */
    virtual FDriftRecievedMatchInviteDelegate& OnReceivedMatchInvite() = 0;
    /**
     * Fired when static data has finished downloading.
     */
    virtual FDriftStaticDataLoadedDelegate& OnStaticDataLoaded() = 0;
    /**
     * Fired when player stats have finished downloading.
     */
    virtual FDriftPlayerStatsLoadedDelegate& OnPlayerStatsLoaded() = 0;
    /**
     * Fired when the player game state has finished downloading.
     */
    virtual FDriftPlayerGameStateLoadedDelegate& OnPlayerGameStateLoaded() = 0;
    /**
     * Fired when the player game state has finished uploading.
     */
    virtual FDriftPlayerGameStateSavedDelegate& OnPlayerGameStateSaved() = 0;
    /**
     * Fired for static data download progress.
     */
    virtual FDriftStaticDataProgressDelegate& OnStaticDataProgress() = 0;
    virtual FDriftGotActiveMatchesDelegate& OnGotActiveMatches() = 0;
    virtual FDriftPlayerNameSetDelegate& OnPlayerNameSet() = 0;

    /**
     * Fired when another player has accepted a friend request.
     */
    virtual FDriftFriendAddedDelegate& OnFriendAdded() = 0;

    /**
     * Fired when a friend has terminated the friendship.
     */
    virtual FDriftFriendRemovedDelegate& OnFriendRemoved() = 0;

    /**
     * Fired when another player sends a friend request our way
    */
    virtual FDriftFriendRequestReceivedDelegate& OnFriendRequestReceived() = 0;

    /**
     * Fired when the root endpoints have been aquired. The user is not yet
     * authenticated at this point.
     */
    virtual FDriftStaticRoutesInitializedDelegate& OnStaticRoutesInitialized() = 0;
    /**
     * Fired when the player gets disconnected, due to error, or user request.
     */
    virtual FDriftPlayerDisconnectedDelegate& OnPlayerDisconnected() = 0;
    /**
     * Fired when the server thinks the version of the client is invalid.
     */
    virtual FDriftGameVersionMismatchDelegate& OnGameVersionMismatch() = 0;
    /**
     * Fired when the request sent to the server is malformed,
     * not valid for the current state, or otherwise invalid.
     */
    virtual FDriftUserErrorDelegate& OnUserError() = 0;
    /**
     * Fired when the server experiences an internal error, or is busy, due to no fault of the user.
     */
    virtual FDriftServerErrorDelegate& OnServerError() = 0;
    /**
     * Fired when the server gets a deprecation notification from the backend.
     */
    virtual FDriftNewDeprecationDelegate OnDeprecation() = 0;

    /**
     * Return the JWT required to authenticate with secondary backend modules
     */
    virtual FString GetJWT() const = 0;
    /**
     * Return the JTI required to pass the current session to a new instance
     */
    virtual FString GetJTI() const = 0;
    /**
     * Return the current Root URL
     */
    virtual FString GetRootURL() const = 0;
    /**
    * Return the current environment specifier
    */
    virtual FString GetEnvironment() const = 0;
    /**
    * Return the game version
    */
    virtual FString GetGameVersion() const = 0;
    /**
    * Return the game build
    */
    virtual FString GetGameBuild() const = 0;
    /**
    * Return API key with version
    */
    virtual FString GetVersionedAPIKey() const = 0;

    /** Fired when received a text message from friend */
    virtual FDriftReceivedMessageDelegate& OnReceivedTextMessage() = 0;

    /** Fired when received a json message from friend */
    virtual FDriftReceivedMessageDelegate& OnReceivedJsonMessage() = 0;

    /** Send a text message to a friend */
    virtual bool SendFriendMessage(int32 FriendId, const FString& Message) = 0;

    /** Send a json message to a friend */
    virtual bool SendFriendMessage(int32 FriendId, class JsonValue&& Message) = 0;

    /** Get the player party manager */
    virtual TSharedPtr<IDriftPartyManager> GetPartyManager() = 0;

    /** Get the matchmaker */
    virtual TSharedPtr<IDriftMatchmaker> GetMatchmaker() = 0;

    /** Get the player lobby manager */
    virtual TSharedPtr<IDriftLobbyManager> GetLobbyManager() = 0;

    /** Get the match placement manager */
    virtual TSharedPtr<IDriftMatchPlacementManager> GetMatchPlacementManager() = 0;

    /** Get the match placement manager */
    virtual TSharedPtr<IDriftSandboxManager> GetSandboxManager() = 0;

    /** Get a handle to the message queue */
    virtual TSharedPtr<IDriftMessageQueue> GetMessageQueue() const = 0;

    /** Return the index of this drift instance */
    virtual int32 GetInstanceIndex() const = 0;

    /** Set the min level of the forwarded logs */
    virtual void SetForwardedLogLevel(ELogVerbosity::Type Level) = 0;

    virtual ~IDriftAPI() {}
};


typedef TSharedPtr<IDriftAPI, ESPMode::ThreadSafe> DriftApiPtr;


class SerializationContext;


USTRUCT(BlueprintType)
struct FBlueprintActiveMatch
{
    GENERATED_BODY()

    FActiveMatch match;
};


USTRUCT(BlueprintType)
struct FBlueprintMatchQueueStatus
{
    GENERATED_BODY()

    FMatchQueueStatus queue;
};


USTRUCT(BlueprintType)
struct FBlueprintLeaderboardEntry
{
    GENERATED_BODY()

    FDriftLeaderboardEntry entry;
};


USTRUCT(BlueprintType)
struct FBlueprintFriend
{
    GENERATED_BODY()

    FDriftFriend entry;
};


USTRUCT(BlueprintType)
struct FBlueprintMatchInvite
{
    GENERATED_BODY()

    FMatchInvite invite;
};


struct FGetMatchesResponseItem
{
    FDateTime create_date;
    FString game_mode;
    int32 machine_id = 0;
    FString machine_url;
    FString map_name;
    int32 match_id = 0;
    FString match_status;
    FString url;
    int32 num_players;
    int32 max_players;
    int32 port;
    FString public_ip;
    FString ref;
    int32 server_id;
    FString server_status;
    FString server_url;
    FString ue4_connection_url;
    FString unique_key;
    FString version;

    FString matchplayers_url;

    bool Serialize(class SerializationContext& context);
};


struct FGetActiveMatchesResponse
{
    TArray<FGetMatchesResponseItem> matches;

    bool Serialize(class SerializationContext& context);
};


struct FMessageQueueEntry
{
    int32 exchange_id;
    // The sending player
    int32 sender_id;
    // Always incrementing message ID
    int32 message_number;

    FString message_id;
    FString exchange;
    // The queue name
    FString queue;

    // Time when message was sent
    FDateTime timestamp;
    // Time when message expires
    FDateTime expires;

    // The actual message content
    JsonValue payload{ rapidjson::kObjectType };

    FMessageQueueEntry()
    {
    }

    FMessageQueueEntry(const FMessageQueueEntry& other)
        : exchange_id(other.exchange_id)
        , sender_id(other.sender_id)
        , message_number(other.message_number)
        , message_id(other.message_id)
        , exchange(other.exchange)
        , queue(other.queue)
        , timestamp(other.timestamp)
        , expires(other.expires)
    {
        payload.CopyFrom(other.payload);
    }

    FMessageQueueEntry(FMessageQueueEntry&& other)
        : exchange_id(MoveTemp(other.exchange_id))
        , sender_id(MoveTemp(other.sender_id))
        , message_number(MoveTemp(other.message_number))
        , message_id(MoveTemp(other.message_id))
        , exchange(MoveTemp(other.exchange))
        , queue(MoveTemp(other.queue))
        , timestamp(MoveTemp(other.timestamp))
        , expires(MoveTemp(other.expires))
    {
        payload = MoveTemp(other.payload);
    }

    bool Serialize(SerializationContext& context);
};
