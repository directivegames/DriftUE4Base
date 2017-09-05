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

#include "CoreUObject.h"

#include "DriftAPI.generated.h"


/**
 * Fired when server registration has completed.
 * bool - success
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FDriftServerRegisteredDelegate, bool);

/**
 * Fired when AddMatch() finishes
 * bool - success
 * int32 - match id
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftMatchAddedDelegate, bool, int32);

/**
 * Fired when UpdateMatch() finishes
 * bool - success
 * int32 - match id
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftMatchUpdatedDelegate, bool, int32);

/**
 * Fired when AddPlayerToMatch() finishes. Use this when you're not the
 * instigator.
 * bool - success
 * int32 - match id
 * int32 - player id
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FDriftPlayerAddedToMatchDelegate, bool, int32, int32);

/**
 * Fired when RemovePlayerFromMatch() finishes. Use this when you're not the
 * instigator.
 * bool - success
 * int32 - match id
 * int32 - player id
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FDriftPlayerRemovedFromMatchDelegate, bool, int32, int32);

/**
 * Fired to notify the original caller when AddPlayerToMatch() finishes.
 * bool - success
 * int32 - match id
 * int32 - player id
 */
DECLARE_DELEGATE_ThreeParams(FDriftPlayerAddedDelegate, bool, int32, int32);

/**
 * Fired to notify the original caller when RemovePlayerFromMatch() finishes.
 * bool - success
 * int32 - match id
 * int32 - player id
 */
DECLARE_DELEGATE_ThreeParams(FDriftPlayerRemovedDelegate, bool, int32, int32);

DECLARE_DELEGATE_TwoParams(FDriftOnMatchAddedDelegate, bool, int32);
DECLARE_DELEGATE_OneParam(FDriftServerStatusUpdatedDelegate, bool);
DECLARE_DELEGATE_TwoParams(FDriftMatchStatusUpdatedDelegate, bool, int32);

struct FAnalyticsEventAttribute;
class IDriftEvent;


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
    virtual void AddMatch(const FString& map_name, const FString& game_mode, int32 num_teams, int32 max_players) = 0;
    virtual void AddMatch(const FString& map_name, const FString& game_mode, int32 num_teams, int32 max_players, const FDriftOnMatchAddedDelegate& delegate) = 0;

    /**
    * Update a server to set it's status for the match maker.
    */
    virtual void UpdateServer(const FString& status, const FString& reason, const FDriftServerStatusUpdatedDelegate& delegate) = 0;

    /**
     * Update a match to set it's status for the match maker. A status of "completed" means the match has ended.
     */
    virtual void UpdateMatch(const FString& status, const FString& reason, const FDriftMatchStatusUpdatedDelegate& delegate) = 0;
    virtual void UpdateMatch(int32 match_id, const FString& status, const FString& reason, const FDriftMatchStatusUpdatedDelegate& delegate) = 0;

    /**
     * Get the match ID if currently hosting a match, or 0.
     */
    virtual int32 GetMatchID() const = 0;
    
    /**
     * Register a player with the current match. This lets the backend know the player has
     * successfully connected to the match.
     */
    virtual void AddPlayerToMatch(int32 player_id, int32 team_id, const FDriftPlayerAddedDelegate& delegate) = 0;
    virtual void AddPlayerToMatch(int32 match_id, int32 team_id, int32 player_id, const FDriftPlayerAddedDelegate& delegate) = 0;

    /**
     * Remove a player from the current match. This should be done if the player disconnects,
     * but the match isn't ending. When the match is set to "completed", players are automatically removed.
     */
    virtual void RemovePlayerFromMatch(int32 player_id, const FDriftPlayerRemovedDelegate& delegate) = 0;
    virtual void RemovePlayerFromMatch(int32 match_id, int32 player_id, const FDriftPlayerRemovedDelegate& delegate) = 0;

    /**
     * Modify a player's counter. Counters are automatically loaded for each player
     * as they are added to the match.
     */
    virtual void ModifyPlayerCounter(int32 player_id, const FString& counter_name, float value, bool absolute) = 0;
    
    /**
     * Get a player's counter. Counters are automatically loaded for each player
     * as they are added to the match.
     */
    virtual bool GetPlayerCounter(int32 player_id, const FString& counter_name, float& value) = 0;
    
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
    int32 match_id;
    int32 num_players;

    FDateTime create_date;
    FString game_mode;
    FString map_name;
    FString match_status;
    FString server_status;
    FString ue4_connection_url;
    FString version;
    
    FString matchplayers_url;
};


struct FMatchesSearch
{
    TArray<FActiveMatch> matches;
    FString ref_filter;
};


struct FMatchQueueStatus
{
    FName status;
    FActiveMatch match;
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
    : playerID{ _playerID }
    , token{ _token }
    , sent{ _sent }
    , expires{ _expires }
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
};


struct FPlayerAuthenticatedInfo
{
    int32 playerId;
    FString playerName;
    EAuthenticationResult result;
    FString error;
    
    FPlayerAuthenticatedInfo(EAuthenticationResult _result, const FString& _error)
    : result{ _result }
    , error{ _error }
    {

    }

    FPlayerAuthenticatedInfo(int32 _playerId, const FString& _playerName)
    : playerId{ _playerId }
    , playerName{ _playerName }
    , result{ EAuthenticationResult::Success }
    {

    }
};


struct FDriftLeaderboardEntry
{
    FString player_name;
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


struct FDriftFriend
{
    int32 playerID;
    FString name;
    EDriftPresence presence;
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


enum class EPlayerIdentityOverrideOption : uint8
{
    DoNotOverrideExistingUserAssociation,
    AssignIdentityToNewUser
};


enum class EAddPlayerIdentityResult : uint8
{
    Success,
    Progress_IdentityAssociatedWithOtherUser,
    Error_FailedToAquireCredentials,
    Error_FailedToAuthenticate,
    Error_FailedToBindNewIdentity,
    Error_UserAlreadyBoundToSameIdentityType,
    Error_Failed
};


DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftPlayerAuthenticatedDelegate, bool, const FPlayerAuthenticatedInfo&);
DECLARE_MULTICAST_DELEGATE_OneParam(FDriftConnectionStateChangedDelegate, EDriftConnectionState);
DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftStaticDataLoadedDelegate, bool, const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftStaticDataProgressDelegate, const FString&, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FDriftGotActiveMatchesDelegate, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FDriftPlayerNameSetDelegate, bool);
DECLARE_MULTICAST_DELEGATE(FDriftStaticRoutesInitializedDelegate);
DECLARE_MULTICAST_DELEGATE_OneParam(FDriftPlayerStatsLoadedDelegate, bool);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FDriftPlayerGameStateLoadedDelegate, ELoadPlayerGameStateResult, const FString&, const FString&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftPlayerGameStateSavedDelegate, bool, const FString&);
DECLARE_DELEGATE_TwoParams(FDriftLeaderboardLoadedDelegate, bool, const FString&);
DECLARE_DELEGATE_OneParam(FDriftFriendsListLoadedDelegate, bool);

DECLARE_MULTICAST_DELEGATE_TwoParams(FDriftFriendPresenceChangedDelegate, int32, EDriftPresence);

DECLARE_DELEGATE_OneParam(FDriftPlayerIdentityContinuationDelegate, EPlayerIdentityOverrideOption);
DECLARE_DELEGATE_TwoParams(FDriftAddPlayerIdentityProgressDelegate, EAddPlayerIdentityResult, const FDriftPlayerIdentityContinuationDelegate&);

DECLARE_MULTICAST_DELEGATE_OneParam(FDriftGameVersionMismatchDelegate, const FString&);

DECLARE_DELEGATE_ThreeParams(FDriftGameStateLoadedDelegate, ELoadPlayerGameStateResult, const FString&, const FString&);
DECLARE_DELEGATE_TwoParams(FDriftGameStateSavedDelegate, bool, const FString&);

DECLARE_MULTICAST_DELEGATE(FDriftPlayerDisconnectedDelegate)
DECLARE_MULTICAST_DELEGATE(FDriftUserErrorDelegate);
DECLARE_MULTICAST_DELEGATE(FDriftServerErrorDelegate);

DECLARE_DELEGATE_TwoParams(FDriftJoinedMatchQueueDelegate, bool, const FMatchQueueStatus&);
DECLARE_DELEGATE_OneParam(FDriftLeftMatchQueueDelegate, bool);
DECLARE_DELEGATE_TwoParams(FDriftPolledMatchQueueDelegate, bool, const FMatchQueueStatus&);

DECLARE_MULTICAST_DELEGATE_OneParam(FDriftRecievedMatchInviteDelegate, const FMatchInvite&);

DECLARE_DELEGATE_OneParam(FDriftLoadPlayerAvatarUrlDelegate, const FString&);


class IDriftAPI : public IDriftServerAPI
{
public:
    /**
     * Client Specific API
     */
    
    /**
    * Authenticate a player using credentialtype defined in config
    * Fires OnGameVersionMismatch if the game version is invalid
    * Fires OnPlayerAuthenticated() when finished.
    */
    virtual void AuthenticatePlayer() = 0;

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
    virtual void AddCount(const FString& counter_name, float value, bool absolute) = 0;
    
    /**
     * Return the value of a counter for the currently authenticated player.
     */
    virtual bool GetCount(const FString& counter_name, float& value) = 0;
    
    /**
     * Post an event for the metrics system
     */
    virtual void AddAnalyticsEvent(const FString& event_name, const TArray<FAnalyticsEventAttribute>& attributes) = 0;
    
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
     * Saves the authenticated player's named game state.
     * Fires delegate when finished.
     */
    virtual void SavePlayerGameState(const FString& name, const FString& gameState, const FDriftGameStateSavedDelegate& delegate) = 0;

    /**
     * Get the global top leaderboard for counter_name. Requires an authenticated player.
     * Returns data in the passed in leaderboard struct
     * Fires delegate when finished
     */
    virtual void GetLeaderboard(const FString& counter_name, const TSharedRef<FDriftLeaderboard>& leaderboard, const FDriftLeaderboardLoadedDelegate& delegate) = 0;

    /**
     * Get the top leaderboard for counter_name filtered on the authenticated player's friends.
     * Returns data in the passed in leaderboard struct
     * Fires delegate when finished
     */
    virtual void GetFriendsLeaderboard(const FString& counter_name, const TSharedRef<FDriftLeaderboard>& leaderboard, const FDriftLeaderboardLoadedDelegate& delegate) = 0;
    
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
	* Load the avatar url of the currently logged in player
	* Fires delegate when finished
	*/
	virtual void LoadPlayerAvatarUrl(const FDriftLoadPlayerAvatarUrlDelegate& delegate) = 0;

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
    int32 port;
    FString public_ip;
    FString ref;
    int32 server_id;
    FString server_status;
    FString server_url;
    FString ue4_connection_url;
    FString version;
    
    FString matchplayers_url;
    
    bool Serialize(class SerializationContext& context);
};


struct FGetActiveMatchesResponse
{
    TArray<FGetMatchesResponseItem> matches;
    
    bool Serialize(class SerializationContext& context);
};


struct FAddPlayerResponse
{
    int32 player_id;
    int32 team_id;
    
    bool Serialize(class SerializationContext& context);
};


struct FGetMatchesResponse
{
    TArray<FGetMatchesResponseItem> matches;
    
    bool Serialize(class SerializationContext& context);
};
