/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2016-2019 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/


#include "DriftSchemas.h"
#include "JsonArchive.h"


bool FDriftEndpointsResponse::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, active_matches)
		&& SERIALIZE_PROPERTY(context, auth)
		&& SERIALIZE_PROPERTY(context, clientlogs)
		&& SERIALIZE_PROPERTY(context, clients)
		&& SERIALIZE_PROPERTY(context, counters)
		&& SERIALIZE_PROPERTY(context, eventlogs)
		&& SERIALIZE_PROPERTY(context, machines)
		&& SERIALIZE_PROPERTY(context, matches)
		&& SERIALIZE_PROPERTY(context, matchqueue)
		&& SERIALIZE_PROPERTY(context, players)
		&& SERIALIZE_PROPERTY(context, root)
		&& SERIALIZE_PROPERTY(context, servers)
		&& SERIALIZE_PROPERTY(context, static_data)
		&& SERIALIZE_PROPERTY(context, user_identities)
		&& SERIALIZE_PROPERTY(context, users)
		&& SERIALIZE_PROPERTY(context, friend_invites)
		&& SERIALIZE_PROPERTY(context, friend_requests)
		&& SERIALIZE_PROPERTY(context, party_invites)
		&& SERIALIZE_PROPERTY(context, parties)

		// Optional
		&& SERIALIZE_PROPERTY(context, my_friends)
		&& SERIALIZE_PROPERTY(context, my_gamestate)
		&& SERIALIZE_PROPERTY(context, my_gamestates)
		&& SERIALIZE_PROPERTY(context, my_messages)
		&& SERIALIZE_PROPERTY(context, my_player_groups)
		&& SERIALIZE_PROPERTY(context, my_player)
		&& SERIALIZE_PROPERTY(context, my_user);
}


bool FUserPassAuthenticationPayload::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, provider)
		&& SERIALIZE_PROPERTY(context, provider_details)
		&& SERIALIZE_PROPERTY(context, automatic_account_creation)
		&& SERIALIZE_PROPERTY(context, username)
		&& SERIALIZE_PROPERTY(context, password);
}


bool FDriftUserInfoResponse::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, user_id)
		&& SERIALIZE_PROPERTY(context, player_id)
		&& SERIALIZE_PROPERTY(context, identity_id)
		&& SERIALIZE_PROPERTY(context, user_name)
		&& SERIALIZE_PROPERTY(context, player_name)
		&& SERIALIZE_PROPERTY(context, jti);
}


bool ClientUpgradeResponse::Serialize(SerializationContext& context)
{
	bool res = SERIALIZE_PROPERTY(context, action);
	SERIALIZE_OPTIONAL_PROPERTY(context, upgrade_url);
	return res;
}


bool FClientRegistrationResponse::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, client_id)
		&& SERIALIZE_PROPERTY(context, player_id)
		&& SERIALIZE_PROPERTY(context, user_id)
		&& SERIALIZE_PROPERTY(context, next_heartbeat_seconds)
		&& SERIALIZE_PROPERTY(context, url)
		&& SERIALIZE_PROPERTY(context, jwt)
		&& SERIALIZE_PROPERTY(context, jti);
}


bool FDriftPlayerResponse::Serialize(SerializationContext& context)
{
	if (context.IsLoading())
	{
		return SERIALIZE_PROPERTY(context, is_online)
			&& SERIALIZE_PROPERTY(context, player_id)
			&& SERIALIZE_PROPERTY(context, player_name)
			&& SERIALIZE_PROPERTY(context, player_url)
			&& SERIALIZE_PROPERTY(context, num_logons)
			&& SERIALIZE_PROPERTY(context, counter_url)
			&& SERIALIZE_PROPERTY(context, countertotals_url)
			&& SERIALIZE_PROPERTY(context, gamestates_url)
			&& SERIALIZE_PROPERTY(context, journal_url)
			&& SERIALIZE_PROPERTY(context, messagequeue_url)
			&& SERIALIZE_PROPERTY(context, messages_url)
			&& SERIALIZE_PROPERTY(context, user_id)
			&& SERIALIZE_PROPERTY(context, user_url)
			&& SERIALIZE_PROPERTY(context, create_date)
			&& SERIALIZE_PROPERTY(context, modify_date)
			&& SERIALIZE_PROPERTY(context, logon_date)
			&& SERIALIZE_PROPERTY(context, status);
	}

	return false;
}


bool FDriftPlayerUpdateResponse::Serialize(SerializationContext& context)
{
	if (context.IsLoading())
	{
		return SERIALIZE_PROPERTY(context, is_online)
			&& SERIALIZE_PROPERTY(context, player_id);
	}

	return false;
}


bool FChangePlayerNamePayload::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, name);
}


bool FCdnInfo::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, cdn)
		&& SERIALIZE_PROPERTY(context, data_root_url);
}


bool FStaticDataResource::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, commit_id)
		&& SERIALIZE_PROPERTY(context, data_root_url)
		&& SERIALIZE_PROPERTY(context, cdn_list)
		&& SERIALIZE_PROPERTY(context, origin)
		&& SERIALIZE_PROPERTY(context, repository)
		&& SERIALIZE_PROPERTY(context, revision);
}


bool FStaticDataResponse::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, static_data_urls);
}


bool FServerRegistrationPayload::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, instance_name)
		&& SERIALIZE_PROPERTY(context, public_ip)
		&& SERIALIZE_PROPERTY(context, port)
		&& SERIALIZE_PROPERTY(context, command_line)
		&& SERIALIZE_PROPERTY(context, pid)
		&& SERIALIZE_PROPERTY(context, status)
		&& SERIALIZE_PROPERTY(context, placement)
		&& SERIALIZE_PROPERTY(context, ref);
}


bool FMatchesPayload::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, server_id)
		&& SERIALIZE_PROPERTY(context, num_players)
		&& SERIALIZE_PROPERTY(context, num_players)
		&& SERIALIZE_PROPERTY(context, max_players)
		&& SERIALIZE_PROPERTY(context, map_name)
		&& SERIALIZE_PROPERTY(context, game_mode)
		&& SERIALIZE_PROPERTY(context, status)
		&& SERIALIZE_PROPERTY(context, num_teams);
}


FName MatchQueueStatusWaitingName{TEXT("waiting")};
FName MatchQueueStatusMatchedName{TEXT("matched")};
FName MatchQueueStatusErrorName{TEXT("error")};


bool FJoinMatchQueuePayload::Serialize(class SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, player_id)
		&& SERIALIZE_PROPERTY(context, ref)
		&& SERIALIZE_PROPERTY(context, placement)
		&& SERIALIZE_PROPERTY(context, token)
		&& SERIALIZE_PROPERTY(context, criteria);
};


bool FMatchQueueResponse::Serialize(class SerializationContext& context)
{
	SERIALIZE_PROPERTY(context, criteria);
	SERIALIZE_PROPERTY(context, match_id);
	SERIALIZE_PROPERTY(context, match_url);
	SERIALIZE_PROPERTY(context, ue4_connection_url);

	return SERIALIZE_PROPERTY(context, player_id)
		&& SERIALIZE_PROPERTY(context, player_url)
		&& SERIALIZE_PROPERTY(context, player_name)
		&& SERIALIZE_PROPERTY(context, status)
		&& SERIALIZE_PROPERTY(context, matchqueueplayer_url)
		&& SERIALIZE_PROPERTY(context, create_date);
}


bool FPlayerGameStatePayload::Serialize(class SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, gamestate);
}


bool FPlayerGameStateResponse::Serialize(class SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, data);
}


bool FCounterModification::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, name)
		&& SERIALIZE_PROPERTY(context, counter_type)
		&& SERIALIZE_PROPERTY(context, timestamp)
		&& SERIALIZE_PROPERTY(context, value)
		&& SERIALIZE_PROPERTY(context, context_id);
}


void FCounterModification::Update(float value_, FDateTime timestamp_)
{
	timestamp = timestamp_;

	if (absolute)
	{
		value = value_;
	}
	else
	{
		value += value_;
	}
}


bool operator==(const FCounterModification& left, const FCounterModification& right)
{
	return left.name == right.name && left.counter_type == right.counter_type && left.absolute == right.absolute;
}


bool FDriftCounterInfo::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, counter_id)
		&& SERIALIZE_PROPERTY(context, name)
		&& SERIALIZE_PROPERTY(context, url);
}


bool FDriftPlayerCounter::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, counter_id)
		&& SERIALIZE_PROPERTY(context, name)
		&& SERIALIZE_PROPERTY(context, total);
}


bool FDriftUserIdentity::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, identity_name)
		&& SERIALIZE_PROPERTY(context, player_id)
		&& SERIALIZE_PROPERTY(context, player_name)
		&& SERIALIZE_PROPERTY(context, player_url);
}


bool FDriftCreatePlayerGroupPayload::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, identity_names)
		&& SERIALIZE_PROPERTY(context, player_ids);
};


bool FDriftCreatePlayerGroupResponse::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, group_name)
		&& SERIALIZE_PROPERTY(context, player_id)
		&& SERIALIZE_PROPERTY(context, players)
		&& SERIALIZE_PROPERTY(context, secret);
}


bool FDriftLeaderboardResponseItem::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, player_id)
		&& SERIALIZE_PROPERTY(context, player_name)
		&& SERIALIZE_PROPERTY(context, total)
		&& SERIALIZE_PROPERTY(context, position);
}


bool FDriftPlayerGameStateInfo::Serialize(SerializationContext& context)
{
	return context.SerializeProperty(TEXT("namespace"), name)
		&& SERIALIZE_PROPERTY(context, gamestate_url)
		&& SERIALIZE_PROPERTY(context, gamestate_id);
}


bool FClientRegistrationPayload::Serialize(SerializationContext& context)
{
	if (context.IsLoading())
	{
		return false;
	}

	bool ok = SERIALIZE_PROPERTY(context, client_type)
		&& SERIALIZE_PROPERTY(context, build)
		&& SERIALIZE_PROPERTY(context, platform_type)
		&& SERIALIZE_PROPERTY(context, app_guid)
		&& SERIALIZE_PROPERTY(context, version)
		&& SERIALIZE_PROPERTY(context, platform_version)
		&& SERIALIZE_PROPERTY(context, platform_info);

	return ok;
}


bool FDriftLogMessage::Serialize(SerializationContext& context)
{
	if (context.IsLoading())
	{
		// Log messages are only written
		return false;
	}

	SERIALIZE_PROPERTY(context, message);
	SERIALIZE_PROPERTY(context, level);
	SERIALIZE_PROPERTY(context, category);

	SERIALIZE_PROPERTY(context, timestamp);

	return true;
}


bool FServerRegistrationResponse::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, heartbeat_url)
		&& SERIALIZE_PROPERTY(context, machine_id)
		&& SERIALIZE_PROPERTY(context, machine_url)
		&& SERIALIZE_PROPERTY(context, server_id)
		&& SERIALIZE_PROPERTY(context, url);
}


bool FAddMatchResponse::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, match_id)
		&& SERIALIZE_PROPERTY(context, url)
		&& SERIALIZE_PROPERTY(context, stats_url)
		&& SERIALIZE_PROPERTY(context, players_url);
}


bool FMachineInfo::Serialize(class SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, create_date)
		&& SERIALIZE_PROPERTY(context, details)
		&& SERIALIZE_PROPERTY(context, instance_id)
		&& SERIALIZE_PROPERTY(context, instance_name)
		// instance_type;
		&& SERIALIZE_PROPERTY(context, machine_id)
		// machine_info;
		&& SERIALIZE_PROPERTY(context, modify_date)
		&& SERIALIZE_PROPERTY(context, placement)
		&& SERIALIZE_PROPERTY(context, realm)
		&& SERIALIZE_PROPERTY(context, private_ip)
		&& SERIALIZE_PROPERTY(context, public_ip)
		&& SERIALIZE_PROPERTY(context, server_count)
		&& SERIALIZE_PROPERTY(context, server_date)
		&& SERIALIZE_PROPERTY(context, status);
}


bool FServerInfo::Serialize(class SerializationContext& context)
{
	return true;
}


bool FTeamInfo::Serialize(class SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, team_id)
		&& SERIALIZE_PROPERTY(context, match_id)
		&& SERIALIZE_PROPERTY(context, create_date)
		&& SERIALIZE_PROPERTY(context, modify_date)
		&& SERIALIZE_PROPERTY(context, name)
		&& SERIALIZE_PROPERTY(context, statistics)
		&& SERIALIZE_PROPERTY(context, details)
		&& SERIALIZE_PROPERTY(context, url);
}


bool FPlayerInfo::Serialize(class SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, matchplayer_url)
		&& SERIALIZE_PROPERTY(context, player_url);
}


bool FMatchInfo::Serialize(class SerializationContext& context)
{
	const auto Result = SERIALIZE_PROPERTY(context, match_id)
		&& SERIALIZE_PROPERTY(context, server_id)
		&& SERIALIZE_PROPERTY(context, create_date)
		&& SERIALIZE_PROPERTY(context, status)
		&& SERIALIZE_PROPERTY(context, num_players)
		&& SERIALIZE_PROPERTY(context, max_players)
		&& SERIALIZE_PROPERTY(context, game_mode)
		&& SERIALIZE_PROPERTY(context, map_name)
		&& SERIALIZE_PROPERTY(context, match_statistics)
		&& SERIALIZE_PROPERTY(context, details)
		&& SERIALIZE_PROPERTY(context, server)
		&& SERIALIZE_PROPERTY(context, server_url)
		&& SERIALIZE_PROPERTY(context, machine_url)
		&& SERIALIZE_PROPERTY(context, teams)
		&& SERIALIZE_PROPERTY(context, matchplayers_url)
		&& SERIALIZE_PROPERTY(context, teams_url)
		&& SERIALIZE_PROPERTY(context, players)
		&& SERIALIZE_PROPERTY(context, url);

	SERIALIZE_OPTIONAL_PROPERTY(context, unique_key);
	SERIALIZE_OPTIONAL_PROPERTY(context, start_date);
	SERIALIZE_OPTIONAL_PROPERTY(context, end_date);
	SERIALIZE_OPTIONAL_PROPERTY(context, machine);

	return Result;
}


bool FDriftUserIdentityPayload::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, link_with_user_jti)
		&& SERIALIZE_PROPERTY(context, link_with_user_id);
}


bool FDriftFriendResponse::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, friend_id)
		&& SERIALIZE_PROPERTY(context, player_url)
		&& SERIALIZE_PROPERTY(context, friendship_url);
}


bool FDriftFriendRequestsResponse::Serialize(SerializationContext& context)
{
	return SERIALIZE_PROPERTY(context, accept_url)
		&& SERIALIZE_PROPERTY(context, create_date)
		&& SERIALIZE_PROPERTY(context, expiry_date)
		&& SERIALIZE_PROPERTY(context, id)
		&& SERIALIZE_PROPERTY(context, issued_by_player_name)
		&& SERIALIZE_PROPERTY(context, issued_by_player_id)
		&& SERIALIZE_PROPERTY(context, issued_by_player_url)
		&& SERIALIZE_PROPERTY(context, issued_to_player_id)
		&& SERIALIZE_PROPERTY(context, issued_to_player_name)
		&& SERIALIZE_PROPERTY(context, issued_to_player_url)
		&& SERIALIZE_PROPERTY(context, modify_date)
		&& SERIALIZE_PROPERTY(context, token);
}
