// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

#include "DriftPrivatePCH.h"
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
