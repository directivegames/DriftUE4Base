/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2016-2018 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/

#pragma once

#include "Containers/UnrealString.h"
#include "JsonArchive.h"
#include "Misc/DateTime.h"
#include "DriftAPI.h"
#include "CoreMinimal.h"
#include "DriftSchemas.generated.h"


class SerializationContext;


/**
 * Response from GET /drift["endpoints"]
 */
struct FDriftEndpointsResponse
{
	// Always available
	FString active_matches;
	FString auth;
	FString clientlogs;
	FString clients;
    FString client_configs;
	FString counters;
	FString eventlogs;
	FString flexmatch_regions;
	FString flexmatch_tickets;
	FString machines;
	FString matches;
	FString matchqueue;
	FString players;
    FString richpresence;
	FString root;
	FString servers;
	FString static_data;
	FString user_identities;
	FString users;
	FString friend_invites;
	FString friend_requests;
	FString parties;
	FString party_invites;
	FString lobbies;
	FString match_placements;
    FString public_match_placements;
    FString sandbox;

	// Templates
	FString template_lobby_member;
	FString template_lobby_members;
	FString template_player_gamestate;

	// Added after authentication
	FString my_flexmatch;
	FString my_flexmatch_ticket;
	FString my_friends;
	FString my_gamestate;
	FString my_gamestates;
	FString my_messages;
	FString my_player_groups;
	FString my_player;
	FString my_user;

	bool Serialize(SerializationContext& context);
};


struct FUserPassAuthenticationPayload
{
	FString provider;
	JsonValue provider_details{rapidjson::kObjectType};
	bool automatic_account_creation{false};

	// TODO: Remove legacy support
	FString username;
	FString password;

	bool Serialize(SerializationContext& context);
};


/**
 * Response from GET /drift["current_user"]
 */
struct FDriftUserInfoResponse
{
	int32 user_id = 0;
	int32 player_id = 0;
	int32 identity_id = 0;
	FString user_name;
	FString player_name;
	FString jti;

	bool Serialize(SerializationContext& context);
};


/**
 * Returned when a request is made with an invalid app version
 */
struct ClientUpgradeResponse
{
	FString action;
	FString message;
	FString upgrade_url;

	bool Serialize(SerializationContext& context);
};


/**
 * Payload for POST to endpoints.clients
 */
struct FClientRegistrationPayload
{
	FString client_type;
	FString build;
	FString platform_type;
	FString app_guid;
	FString version;
	FString platform_version;
	JsonValue platform_info{rapidjson::kObjectType};

	bool Serialize(SerializationContext& context);
};


/**
 * Response from POST to endpoints.clients
 */
struct FClientRegistrationResponse
{
	int32 client_id = 0;
	int32 player_id = 0;
	int32 user_id = 0;
	int32 next_heartbeat_seconds = 0;
	FString url;
	FString jwt;
	FString jti;

	bool Serialize(SerializationContext& context);
};

/**
 * Response from GET endpoints.richpresence
 */
struct FRichPresenceResponse
{
    FString game_mode;
    FString map_name;
    bool is_online;
    bool is_in_game;

    friend bool operator==(const FRichPresenceResponse& Lhs, const FRichPresenceResponse& RHS)
    {
        return Lhs.game_mode == RHS.game_mode
            && Lhs.map_name == RHS.map_name
            && Lhs.is_online == RHS.is_online
            && Lhs.is_in_game == RHS.is_in_game;
    }

    friend bool operator!=(const FRichPresenceResponse& Lhs, const FRichPresenceResponse& RHS)
    {
        return !(Lhs == RHS);
    }

    bool Serialize(SerializationContext& context);
};

/**
 * Response from GET endpoints.my_player
 * Array item response from GET endpoints.players
 */
struct FDriftPlayerResponse
{
	bool is_online = false; // Deprecated: See RichPresence

	int32 player_id = 0;
	int32 num_logons = 0;
	int32 user_id = 0;
    FString player_uuid;

	FString player_name;
	FString player_url;

	FString counter_url;
	FString countertotals_url;

	FString gamestates_url;
	FString journal_url;

	FString messagequeue_url;
	FString messages_url;

	FString user_url;

	FString status;

	FDateTime create_date;
	FDateTime modify_date;
	FDateTime logon_date;

	bool Serialize(SerializationContext& context);
};


struct FDriftUserIdentityPayload
{
	FString link_with_user_jti;
	int32 link_with_user_id = 0;

	bool Serialize(SerializationContext& context);
};


struct FDriftPlayerUpdateResponse
{
	bool is_online = false;
	int32 player_id = 0;

	bool Serialize(SerializationContext& context);
};


struct FChangePlayerNamePayload
{
	FString name;

	bool Serialize(SerializationContext& context);
};

/*
 * Serialization of this from JSON to the struct, is handled by FJSonObjectConverter instead of JsonArchive
 */
USTRUCT()
struct FDriftClientConfigListResponse
{
    GENERATED_BODY();

    UPROPERTY()
    TMap<FString, FString> client_configs;
};


struct FCdnInfo
{
	FString cdn;
	FString data_root_url;

	bool Serialize(SerializationContext& context);
};


struct FStaticDataResource
{
	FString commit_id;
	FString data_root_url;
	TArray<FCdnInfo> cdn_list;
	FString origin;
	FString repository;
	FString revision;

	bool Serialize(SerializationContext& context);
};


struct FStaticDataResponse
{
	TArray<FStaticDataResource> static_data_urls;

	bool Serialize(SerializationContext& context);
};


struct FServerRegistrationPayload
{
	int32 port = 0;
	int32 pid = 0;
	FString instance_name;
	FString public_ip;
	FString command_line;
	FString status;
	FString placement;
	FString ref;

	bool Serialize(class SerializationContext& context);
};



struct FMatchesPayload
{
	int32 server_id = 0;
	int32 num_players = 0;
	int32 max_players = 0;
	int32 num_teams = 0;
	TArray<FString> team_names;
	FString map_name;
	FString game_mode;
	FString status;

	bool Serialize(class SerializationContext& context);
};


struct FJoinMatchQueuePayload
{
	int32 player_id = 0;
	FString ref;
	FString placement;
	FString token;
	JsonValue criteria{rapidjson::kObjectType};

	bool Serialize(class SerializationContext& context);
};


extern FName MatchQueueStatusWaitingName;
extern FName MatchQueueStatusMatchedName;
extern FName MatchQueueStatusErrorName;


struct FMatchQueueResponse
{
	int32 player_id = 0;
	int32 match_id = 0;
	FString player_url;
	FString player_name;
	FString match_url;
	FName status;
	FString matchqueueplayer_url;
	FString ue4_connection_url;
	FDateTime create_date;
	JsonValue criteria{rapidjson::kObjectType};

	bool Serialize(class SerializationContext& context);
};


/**
 * Payload for PUT gamestates
 */
struct FPlayerGameStatePayload
{
	JsonValue gamestate{rapidjson::kObjectType};

	bool Serialize(class SerializationContext& context);
};


/**
 * Response from GET gamestate
 */
struct FPlayerGameStateResponse
{
	JsonValue data;

	bool Serialize(class SerializationContext& context);
};


/**
 * Array item payload for PUT player.counter_url
 */
struct FCounterModification
{
	int32 context_id = 0;
	float value;
	FString name;
	FString counter_type;
	FDateTime timestamp;

	bool absolute;

	bool Serialize(SerializationContext& context);
	void Update(float value, FDateTime timestamp);
};


bool operator ==(const FCounterModification& left, const FCounterModification& right);


/**
 * Array item response from GET endpoints.counters
 */
struct FDriftCounterInfo
{
	int32 counter_id = -1;
	FString name;
	FString url;

	bool Serialize(SerializationContext& context);
};


bool operator ==(const FDriftCounterInfo& left, const FDriftCounterInfo& right);


/**
 * Array item response from GET endpoints.my_player.counter_url
 */
struct FDriftPlayerCounter
{
	FDriftPlayerCounter() = default;


	FDriftPlayerCounter(int32 counter_id_, float total_, FString name_)
		: counter_id{counter_id_}
		, total{total_}
		, name{name_}
	{
	}


	int32 counter_id = -1;
	float total = -1;
	FString name;

	bool Serialize(SerializationContext& context);
};


/**
 * Payload for PUT endpoints.my_player_groups
 */
struct FDriftCreatePlayerGroupPayload
{
	TArray<FString> identity_names;
	TArray<int32> player_ids;

	bool Serialize(SerializationContext& context);
};


/**
 * Response from PUT endpoints.my_player_groups
 */
struct FDriftCreatePlayerGroupResponse
{
	int32 player_id = 0;
	FString group_name;
	FString secret;
	TArray<FDriftUserIdentity> players;

	bool Serialize(SerializationContext& context);
};


/**
 * Array item response from GET counter
 */
struct FDriftLeaderboardResponseItem
{
	int32 player_id = 0;
	int32 position = 0;
	float total;
	FString player_name;

	bool Serialize(SerializationContext& context);
};


/**
 * Array item response from GET endpoints.my_gamestates
 */
struct FDriftPlayerGameStateInfo
{
	int32 gamestate_id = 0;
	FString name;
	FString gamestate_url;

	bool Serialize(SerializationContext& context);
};


/**
 * Array item payload for POST endpoints.clientlogs
 */
struct FDriftLogMessage
{
	FString message;
	FString level;
	FName category;
	FDateTime timestamp;
    int32 count;
    FDateTime last_entry_timestamp;


	FDriftLogMessage()
	{
	}


    FDriftLogMessage(const FString& _message, const FString& _level, const FName& _category
        , const FDateTime& _timestamp)
        : message(_message)
        , level(_level)
        , category(_category)
        , timestamp(_timestamp)
        , count(1)
        , last_entry_timestamp(_timestamp)
	{
	}

	bool Serialize(SerializationContext& context);
};


struct FServerRegistrationResponse
{
	FString heartbeat_url;
	int32 machine_id = 0;
	FString machine_url;
	int32 server_id = 0;
	FString url;

	bool Serialize(class SerializationContext& context);
};


struct FAddMatchResponse
{
	int32 match_id = 0;
	FString url;
	FString stats_url;
	FString players_url;

	bool Serialize(class SerializationContext& context);
};

/**
 * Response from adding a player to a match
 */

struct FAddPlayerToMatchResponse
{
	int32 match_id = 0;
	int32 player_id = 0;
	int32 team_id = 0;

	FString url;

	bool Serialize(SerializationContext& context);
};


struct FServerInfo
{
	bool Serialize(class SerializationContext& context);
};


struct FMachineInfo
{
	FDateTime create_date;
	JsonValue details{rapidjson::kObjectType};
	int32 instance_id = 0;
	FString instance_name;
	// instance_type;
	int32 machine_id = 0;
	// machine_info;
	FDateTime modify_date;
	FString placement;
	FString realm;
	FString private_ip;
	FString public_ip;
	int32 server_count = 0;
	FDateTime server_date;
	FString status;

	bool Serialize(class SerializationContext& context);
};


struct FDriftMatchTeamInfo
{
	int32 team_id = 0;
	int32 match_id = 0;

	FDateTime create_date;
	FDateTime modify_date;

	FString name;
	JsonValue statistics{rapidjson::kObjectType};
	JsonValue details{rapidjson::kObjectType};

	FString url;

	bool Serialize(class SerializationContext& context);
};


struct FDriftMatchPlayerInfo
{
    int32 id = 0;
    int32 match_id = 0;
    int32 player_id = 0;
    int32 team_id = 0;

    FDateTime create_date;
    FDateTime join_date;
    FDateTime leave_date;
    FDateTime modify_date;

    FString player_name;
    FString status;
    int32 num_joins = 0;
    int32 seconds = 0;

    JsonValue details{rapidjson::kObjectType};
    JsonValue statistics{rapidjson::kObjectType};

    FString matchplayer_url;
    FString player_url;

	bool Serialize(class SerializationContext& context);
};


struct FMatchInfo
{
	int32 match_id = 0;
	int32 server_id = 0;

	FDateTime create_date;
	FDateTime start_date;
	FDateTime end_date;
	FString status;

	int32 num_players = 0;
	int32 max_players = 0;

	FString game_mode;
	FString map_name;
	FString unique_key;

	JsonValue match_statistics{rapidjson::kObjectType};
	JsonValue details{rapidjson::kObjectType};

	FServerInfo server;
	FString server_url;
	FMachineInfo machine;
	FString machine_url;

	TArray<FDriftMatchTeamInfo> teams;

	FString matchplayers_url;
	FString teams_url;

	TArray<FDriftMatchPlayerInfo> players;

	FString url;

	bool Serialize(class SerializationContext& context);
};

struct FDriftGetMatchesResponse
{
    TArray<FMatchInfo> items;
    int32 total;
    int32 page;
    int32 pages;
    int32 per_page;

    bool Serialize(class SerializationContext& context);
};


struct FDriftFriendResponse
{
	int32 friend_id = 0;
	FString player_url;
	FString friendship_url;

	bool Serialize(class SerializationContext& context);
};


struct FDriftFriendRequestsResponse
{
	FString accept_url;
	FDateTime create_date;
	FDateTime expiry_date;
	int32 id = 0;
	FString issued_by_player_name;
	int32 issued_by_player_id = 0;
	FString issued_by_player_url;
	int32 issued_to_player_id = 0;
	FString issued_to_player_name;
	FString issued_to_player_url;
	FDateTime modify_date;
	FString token;

	bool Serialize(class SerializationContext& context);
};

struct FDriftFlexmatchLatencySchema
{
	JsonValue latencies{rapidjson::kObjectType};

	bool Serialize(class SerializationContext& context);
};

struct FDriftFlexmatchTicketPostResponse
{
	FString ticket_url;
	FString ticket_id;
    FString ticket_status;
    FString matchmaker;

	bool Serialize(class SerializationContext& context);
};

struct FDriftFlexmatchTicketDeleteResponse
{
	FString status;

	bool Serialize(class SerializationContext& context);
};
