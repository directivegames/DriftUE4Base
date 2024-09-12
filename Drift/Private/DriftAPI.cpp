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

#include "DriftAPI.h"
#include "JsonArchive.h"


bool FGetActiveMatchesResponse::Serialize(SerializationContext& context)
{
    return SERIALIZE_PROPERTY(context, matches);
}


bool FRichPresenceResult::Serialize(SerializationContext& context)
{
    const bool result =  SERIALIZE_PROPERTY(context, game_mode)
        && SERIALIZE_PROPERTY(context, map_name)
        && SERIALIZE_PROPERTY(context, is_online)
        && SERIALIZE_PROPERTY(context, game_mode);
    return result;
}

bool FGetMatchesResponseItem::Serialize(SerializationContext& context)
{
    const bool result = SERIALIZE_PROPERTY(context, create_date)
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
    SERIALIZE_OPTIONAL_PROPERTY(context, unique_key);
    return result;
}


bool FMessageQueueEntry::Serialize(SerializationContext& context)
{
    return SERIALIZE_PROPERTY(context, exchange_id)
        && SERIALIZE_PROPERTY(context, sender_id)
        && SERIALIZE_PROPERTY(context, message_number)
        && SERIALIZE_PROPERTY(context, message_id)
        && SERIALIZE_PROPERTY(context, exchange)
        && SERIALIZE_PROPERTY(context, queue)
        && SERIALIZE_PROPERTY(context, timestamp)
        && SERIALIZE_PROPERTY(context, expires)
        && SERIALIZE_PROPERTY(context, payload);
}
