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

#include "DriftAPI.h"

#include "DriftFriendsBlueprintLibrary.generated.h"


UCLASS()
class UDriftFriendsBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    /**
     * Get a list of friends
     */
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "worldContextObject"), Category = "Drift|Friends")
    static bool GetFriendsList(UObject* worldContextObject, TArray<FBlueprintFriend>& friends);

    /**
     * Update friends list
     * Individual events will be fired if anything changed
     */
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "worldContextObject"), Category = "Drift|Friends")
    static void UpdateFriendsList(UObject* worldContextObject);

    UFUNCTION(BlueprintPure, Category = "Drift|Friends")
    static FString GetNickname(const FBlueprintFriend& entry);

    UFUNCTION(BlueprintPure, Category = "Drift|Friends")
    static int32 GetFriendID(const FBlueprintFriend& entry);
    
    UFUNCTION(BlueprintPure, Category = "Drift|Friends")
    static EDriftPresence GetFriendPresence(const FBlueprintFriend& entry);
};
