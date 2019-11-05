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

#include "Kismet/BlueprintFunctionLibrary.h"

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
