// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#pragma once

#include "OnlineBlueprintCallProxyBase.h"
#include "CommonDelegates.h"

#include "AddPlayerProxy.generated.h"


UCLASS()
class UAddPlayerProxy : public UOnlineBlueprintCallProxyBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FKaleoEmptySuccessDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FKaleoErrorDelegate OnError;

    UFUNCTION(BlueprintCallable, Category = "Drift", meta = (DisplayName = "Add Player To Match", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static UAddPlayerProxy* AddPlayer(UObject* worldContextObject, int32 player_id, int32 team_id = 0);

    void Activate() override;

    ~UAddPlayerProxy();

    int32 player_id;
    int32 team_id;
    
private:
    void OnCompleted(bool success);
    
    UObject* worldContextObject;
};
