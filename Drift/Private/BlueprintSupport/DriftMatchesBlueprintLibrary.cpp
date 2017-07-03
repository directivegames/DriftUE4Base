// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

#include "DriftPrivatePCH.h"
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
    return FBlueprintActiveMatch{ status.queue.match };
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
