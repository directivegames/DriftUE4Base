// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#pragma once

#include "DriftAPI.h"
#include "Kismet/BlueprintFunctionLibrary.h"

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
