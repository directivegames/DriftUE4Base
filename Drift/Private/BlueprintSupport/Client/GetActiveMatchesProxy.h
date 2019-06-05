// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#pragma once

#include "Net/OnlineBlueprintCallProxyBase.h"
#include "DriftAPI.h"
#include "DriftAPI.h"
#include "CommonDelegates.h"

#include "GetActiveMatchesProxy.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGetActiveMatchesDelegate, const TArray<FBlueprintActiveMatch>&, matches);


UCLASS()
class UGetActiveMatchesProxy : public UOnlineBlueprintCallProxyBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Drift|Matches", meta = (DisplayName = "Get Active Matches", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static UGetActiveMatchesProxy* GetActiveMatches(UObject* worldContextObject, FString ref_filter);

    UFUNCTION(BlueprintPure, Category = "Drift|Matches")
    static FString GetGameMode(const FBlueprintActiveMatch& entry);
    
    UFUNCTION(BlueprintPure, Category = "Drift|Matches")
    static FString GetMapName(const FBlueprintActiveMatch& entry);
    
    UFUNCTION(BlueprintPure, Category = "Drift|Matches")
    static FString GetMatchStatus(const FBlueprintActiveMatch& entry);
    
    UFUNCTION(BlueprintPure, Category = "Drift|Matches")
    static int32 GetNumPlayers(const FBlueprintActiveMatch& entry);
    
    UFUNCTION(BlueprintPure, Category = "Drift|Matches")
    static FString GetServerStatus(const FBlueprintActiveMatch& entry);
    
    UPROPERTY(BlueprintAssignable)
    FGetActiveMatchesDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FGetActiveMatchesDelegate OnError;

    void Activate() override;

    UGetActiveMatchesProxy(const FObjectInitializer& oi);
    virtual ~UGetActiveMatchesProxy();

    FString ref_filter;

private:
    void OnCompleted(bool success);

    UObject* worldContextObject;
    TSharedPtr<FMatchesSearch> search;
    
    FDelegateHandle onGotActiveMatchesHandle;
};
