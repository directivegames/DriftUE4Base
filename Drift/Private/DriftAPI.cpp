// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#include "DriftPrivatePCH.h"
#include "DriftAPI.h"
#include "JsonArchive.h"


bool FGetActiveMatchesResponse::Serialize(SerializationContext& context)
{
    return SERIALIZE_PROPERTY(context, matches);
}


bool FGetMatchesResponseItem::Serialize(SerializationContext& context)
{
    return SERIALIZE_PROPERTY(context, create_date)
        && SERIALIZE_PROPERTY(context, game_mode)
        && SERIALIZE_PROPERTY(context, map_name)
        && SERIALIZE_PROPERTY(context, match_id)
        && SERIALIZE_PROPERTY(context, num_players)
		&& SERIALIZE_PROPERTY(context, max_players)
        && SERIALIZE_PROPERTY(context, match_status)
        && SERIALIZE_PROPERTY(context, url)
        && SERIALIZE_PROPERTY(context, server_status)
        && SERIALIZE_PROPERTY(context, version)
        && SERIALIZE_PROPERTY(context, ue4_connection_url)
        && SERIALIZE_PROPERTY(context, matchplayers_url)
        && SERIALIZE_PROPERTY(context, ref);
}
