

#pragma once

#include "CommonDelegates.h"
#include "DriftAPI.h"

#include "Kismet/BlueprintAsyncActionBase.h"

#include "LeaderboardsProxy.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKaleoProxyLeaderboardLoadedDelegate, const TArray<FBlueprintLeaderboardEntry>&, entries);


UCLASS()
class ULeaderboardsProxy : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Drift|Leaderboards", meta = (DisplayName = "Get Leaderboard", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static ULeaderboardsProxy* GetLeaderboard(UObject* worldContextObject, const FString& group, const FString& name);
    
    UFUNCTION(BlueprintCallable, Category = "Drift|Leaderboards", meta = (DisplayName = "Get Friends Leaderboard", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static ULeaderboardsProxy* GetFriendsLeaderboard(UObject* worldContextObject, const FString& group, const FString& name);
    
    UFUNCTION(BlueprintPure, Category = "Drift|Leaderboards")
    static int32 GetPosition(const FBlueprintLeaderboardEntry& entry);

    UFUNCTION(BlueprintPure, Category = "Drift|Leaderboards")
    static FString GetPlayerName(const FBlueprintLeaderboardEntry& entry);

    UFUNCTION(BlueprintPure, Category = "Drift|Leaderboards")
    static float GetValue(const FBlueprintLeaderboardEntry& entry);

    UPROPERTY(BlueprintAssignable)
	FKaleoProxyLeaderboardLoadedDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FKaleoProxyLeaderboardLoadedDelegate OnError;

    void Activate() override;

	ULeaderboardsProxy(const FObjectInitializer& oi);
	~ULeaderboardsProxy();

    FString group_;
    FString name_;

private:
    void OnCompleted(bool success, const FString& name);

    UObject* worldContextObject_;

    bool friends_ = false;

    TSharedPtr<FDriftLeaderboard> leaderboard_;
};
