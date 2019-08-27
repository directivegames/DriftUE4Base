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

#include "DriftMetricsBlueprintLibrary.h"

#include "DriftAPI.h"
#include "DriftUtils.h"


void UDriftMetricsBlueprintLibrary::AddCount(UObject* worldContextObject, const FString& group_name, const FString& counter_name, float value, bool absolute)
{
	FDriftWorldHelper helper{ worldContextObject };
	auto kc = helper.GetInstance();
	if (kc)
	{
		kc->AddCount(FString::Printf(TEXT("%s.%s"), *group_name, *counter_name), value, absolute);
	}
}


void UDriftMetricsBlueprintLibrary::GetPlayerStat(UObject* worldContextObject, float& value, const FString& group_name, const FString& counter_name)
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
    if (kc)
    {
        kc->GetCount(FString::Printf(TEXT("%s.%s"), *group_name, *counter_name), value);
    }
}


void UDriftMetricsBlueprintLibrary::ModifyPlayerCounter(UObject* worldContextObject, int32 player_id, const FString& group_name, const FString& counter_name, float value, bool absolute)
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
    if (kc)
    {
        kc->ModifyPlayerCounter(player_id, FString::Printf(TEXT("%s.%s"), *group_name, *counter_name), value, absolute);
    }
}


void UDriftMetricsBlueprintLibrary::GetPlayerCounter(UObject* worldContextObject, int32 player_id, float& value, const FString& group_name, const FString& counter_name)
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
    if (kc)
    {
        kc->GetPlayerCounter(player_id, FString::Printf(TEXT("%s.%s"), *group_name, *counter_name), value);
    }
}


void UDriftMetricsBlueprintLibrary::FlushCounters(UObject *worldContextObject)
{
    FDriftWorldHelper helper{ worldContextObject };
    auto kc = helper.GetInstance();
    if (kc)
    {
        kc->FlushCounters();
    }
}
