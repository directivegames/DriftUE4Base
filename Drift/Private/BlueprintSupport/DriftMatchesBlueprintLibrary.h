// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#pragma once

#include "DriftAPI.h"

#include "DriftMatchesBlueprintLibrary.generated.h"


UCLASS()
class UDriftMatchesBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "worldContextObject"), Category = "Drift|Matches")
    static void JoinMatch(UObject* worldContextObject, APlayerController* playerController, FBlueprintActiveMatch match);

    UFUNCTION(BlueprintPure, Category = "Drift|Matches")
    static FString GetStatus(const FBlueprintMatchQueueStatus& entry);

    UFUNCTION(BlueprintPure, Category = "Drift|Matches")
    static FBlueprintActiveMatch GetMatch(const FBlueprintMatchQueueStatus& entry);

    UFUNCTION(BlueprintPure, Category = "Drift|Matches")
    static int32 GetInvitingPlayerID(const FBlueprintMatchInvite& invite);

    UFUNCTION(BlueprintPure, meta = (WorldContext = "worldContextObject"), Category = "Drift|Matches")
    static FString GetInvitingPlayerName(UObject* worldContextObject, const FBlueprintMatchInvite& invite);
    
    UFUNCTION(BlueprintPure, Category = "Drift|Matches")
    static int32 GetExpiresInSeconds(const FBlueprintMatchInvite& invite);
};
