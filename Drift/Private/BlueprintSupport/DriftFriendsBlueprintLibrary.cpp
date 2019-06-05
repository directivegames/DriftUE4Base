// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#include "DriftPrivatePCH.h"
#include "DriftFriendsBlueprintLibrary.h"
#include "DriftAPI.h"
#include "DriftUtils.h"


bool UDriftFriendsBlueprintLibrary::GetFriendsList(UObject* worldContextObject, TArray<FBlueprintFriend>& friends)
{
	FDriftWorldHelper helper{ worldContextObject };
	auto kc = helper.GetInstance();
	if (kc)
	{
        TArray<FDriftFriend> list;
		if (kc->GetFriendsList(list))
        {
            for (const auto& item : list)
            {
                friends.Add(FBlueprintFriend{ item });
            }
            return true;
        }
	}
    return false;
}


void UDriftFriendsBlueprintLibrary::UpdateFriendsList(UObject *worldContextObject)
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
    if (kc)
    {
        kc->UpdateFriendsList();
    }
}


FString UDriftFriendsBlueprintLibrary::GetNickname(const FBlueprintFriend& entry)
{
    return entry.entry.name;
}


int32 UDriftFriendsBlueprintLibrary::GetFriendID(const FBlueprintFriend& entry)
{
    return entry.entry.playerID;
}


EDriftPresence UDriftFriendsBlueprintLibrary::GetFriendPresence(const FBlueprintFriend& entry)
{
    return entry.entry.presence;
}
