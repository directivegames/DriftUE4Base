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

#include "DriftMatchesBlueprintLibrary.h"

#include "DriftAPI.h"
#include "DriftUtils.h"


void UDriftMatchesBlueprintLibrary::JoinMatch(UObject* worldContextObject, APlayerController* playerController, FBlueprintActiveMatch match)
{
    if (playerController)
    {
        if (auto drift = FDriftWorldHelper(worldContextObject).GetInstance())
        {
            if (drift->GetMatchQueueState() == EMatchQueueState::Matched)
            {
                drift->ResetMatchQueue();
            }
        }
        playerController->ClientTravel(match.match.ue4_connection_url, ETravelType::TRAVEL_Absolute);
    }
}


FString UDriftMatchesBlueprintLibrary::GetStatus(const FBlueprintMatchQueueStatus& status)
{
    return status.queue.status.ToString();
}


FBlueprintActiveMatch UDriftMatchesBlueprintLibrary::GetMatch(const FBlueprintMatchQueueStatus& status)
{
    FBlueprintActiveMatch match;
    match.match.create_date = status.queue.match.create_date;
    match.match.ue4_connection_url = status.queue.match.ue4_connection_url;
    match.match.match_id = status.queue.match.match_id;
    return match;
}


int32 UDriftMatchesBlueprintLibrary::GetInvitingPlayerID(const FBlueprintMatchInvite& invite)
{
    return invite.invite.playerID;
}


FString UDriftMatchesBlueprintLibrary::GetInvitingPlayerName(UObject* worldContextObject, const FBlueprintMatchInvite& invite)
{
    if (auto drift = FDriftWorldHelper(worldContextObject).GetInstance())
    {
        return drift->GetFriendName(invite.invite.playerID);
    }
    return TEXT("");
}


int32 UDriftMatchesBlueprintLibrary::GetExpiresInSeconds(const FBlueprintMatchInvite& invite)
{
    return (invite.invite.expires - FDateTime::UtcNow()).GetTotalSeconds();
}
