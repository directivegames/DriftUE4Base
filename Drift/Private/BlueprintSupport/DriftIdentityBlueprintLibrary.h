// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#pragma once

#include "DriftIdentityBlueprintLibrary.generated.h"


UCLASS()
class UDriftIdentityBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    /**
     * Get the player's name
     */
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "worldContextObject"), Category = "Drift|Player")
    static void GetPlayerName(UObject* worldContextObject, FString& name);

    /**
     * Get the player's ID
     */
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "worldContextObject"), Category = "Drift|Player")
    static int32 GetPlayerID(UObject* worldContextObject);
   
    /**
     * Get the name of the current auth provider, such as Steam, GameCenter, etc.
     */
    UFUNCTION(BlueprintCallable, meta = (WorldContext = "worldContextObject"), Category = "Drift|Player")
    static FString GetAuthProviderName(UObject* worldContextObject);
};
