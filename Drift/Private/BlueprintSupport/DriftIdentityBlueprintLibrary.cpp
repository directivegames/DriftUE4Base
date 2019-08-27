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

#include "DriftIdentityBlueprintLibrary.h"

#include "DriftAPI.h"
#include "DriftUtils.h"


void UDriftIdentityBlueprintLibrary::GetPlayerName(UObject* worldContextObject, FString& name)
{
	FDriftWorldHelper helper{ worldContextObject };
	auto kc = helper.GetInstance();
	if (kc)
	{
		name = kc->GetPlayerName();
	}
}


int32 UDriftIdentityBlueprintLibrary::GetPlayerID(UObject* worldContextObject)
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
    if (kc)
    {
        return kc->GetPlayerID();
    }
    return 0;
}


FString UDriftIdentityBlueprintLibrary::GetAuthProviderName(UObject* worldContextObject)
{
    FDriftWorldHelper helper{ worldContextObject };
    if (auto kc = helper.GetInstance())
    {
        return kc->GetAuthProviderName();
    }
    
    return TEXT("");
}
