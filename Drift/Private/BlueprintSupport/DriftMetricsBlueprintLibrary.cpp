// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#include "DriftPrivatePCH.h"

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
