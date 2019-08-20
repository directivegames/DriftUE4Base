

#include "LeaderboardsProxy.h"
#include "DriftUtils.h"
#include "DriftBase.h"


ULeaderboardsProxy* ULeaderboardsProxy::GetLeaderboard(UObject* worldContextObject, const FString& group, const FString& name)
{
	auto proxy = NewObject<ULeaderboardsProxy>();
	proxy->group_ = group;
    proxy->name_ = name;
	proxy->worldContextObject_ = worldContextObject;
	return proxy;
}


ULeaderboardsProxy* ULeaderboardsProxy::GetFriendsLeaderboard(UObject* worldContextObject, const FString& group, const FString& name)
{
    auto proxy = NewObject<ULeaderboardsProxy>();
    proxy->group_ = group;
    proxy->name_ = name;
    proxy->worldContextObject_ = worldContextObject;
    proxy->friends_ = true;
    return proxy;
}


int32 ULeaderboardsProxy::GetPosition(const FBlueprintLeaderboardEntry& entry)
{
    return entry.entry.position;
}


FString ULeaderboardsProxy::GetPlayerName(const FBlueprintLeaderboardEntry& entry)
{
    return entry.entry.player_name;
}


float ULeaderboardsProxy::GetValue(const FBlueprintLeaderboardEntry& entry)
{
    return entry.entry.value;
}


void ULeaderboardsProxy::Activate()
{
    leaderboard_ = MakeShareable(new FDriftLeaderboard());
	FDriftWorldHelper helper{ worldContextObject_ };
	auto kc = helper.GetInstance();
	if (kc)
	{
        if (friends_)
        {
            kc->GetFriendsLeaderboard(FString::Printf(TEXT("%s.%s"), *group_, *name_), leaderboard_.ToSharedRef(), FDriftLeaderboardLoadedDelegate::CreateUObject(this, &ThisClass::OnCompleted));
        }
        else
        {
            kc->GetLeaderboard(FString::Printf(TEXT("%s.%s"), *group_, *name_), leaderboard_.ToSharedRef(), FDriftLeaderboardLoadedDelegate::CreateUObject(this, &ThisClass::OnCompleted));
        }
	}
}


void ULeaderboardsProxy::OnCompleted(bool success, const FString& name)
{
    if (!name_.Contains(group_) || !name.Contains(name_))
    {
        // Not for us
        return;
    }

    TArray<FBlueprintLeaderboardEntry> result;
    if (success)
    {
        for (const auto& entry : leaderboard_->rows)
        {
            FBlueprintLeaderboardEntry row;
            row.entry = entry;
            result.Add(row);
        }
        OnSuccess.Broadcast(result);
    }
    else
    {
        OnError.Broadcast(result);
    }
}


ULeaderboardsProxy::ULeaderboardsProxy(const FObjectInitializer& oi)
: Super{ oi }
{
    
}


ULeaderboardsProxy::~ULeaderboardsProxy()
{
    // TODO: Cancel request?
}
