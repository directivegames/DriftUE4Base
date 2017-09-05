// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#pragma once

#include "OnlineBlueprintCallProxyBase.h"
#include "DriftAPI.h"
#include "CommonDelegates.h"

#include "AddMatchProxy.generated.h"


UCLASS()
class UAddMatchProxy : public UOnlineBlueprintCallProxyBase
{
    GENERATED_BODY()

public:

    /* Event which triggers when the content has been retrieved */
    UPROPERTY(BlueprintAssignable)
    FKaleoEmptySuccessDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FKaleoErrorDelegate OnError;

    UFUNCTION(BlueprintCallable, Category = "Drift", meta = (DisplayName = "Add Match", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static UAddMatchProxy* AddMatch(UObject* worldContextObject, int32 max_players, FString map_name, FString game_mode, int32 num_teams = 2);

    void Activate() override;

    UAddMatchProxy();

    int32 max_players;
    FString map_name;
    FString game_mode;
    int32 num_teams;
    
private:
    void OnCompleted(bool success, int32 match_id);
    
    UObject* worldContextObject;

    FDelegateHandle onMatchAddedHandle;
};
