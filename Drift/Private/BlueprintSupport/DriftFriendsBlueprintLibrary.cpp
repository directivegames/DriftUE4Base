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
