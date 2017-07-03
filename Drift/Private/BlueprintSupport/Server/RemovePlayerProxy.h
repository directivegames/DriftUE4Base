// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#pragma once

#include "OnlineBlueprintCallProxyBase.h"
#include "CommonDelegates.h"

#include "RemovePlayerProxy.generated.h"


UCLASS()
class URemovePlayerProxy : public UOnlineBlueprintCallProxyBase
{
    GENERATED_BODY()

public:
    /* Event which triggers when the content has been retrieved */
    UPROPERTY(BlueprintAssignable)
    FKaleoEmptySuccessDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FKaleoErrorDelegate OnError;

    UFUNCTION(BlueprintCallable, Category = "Drift", meta = (DisplayName = "Remove Player From Match", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static URemovePlayerProxy* RemovePlayer(UObject* worldContextObject, int32 player_id);

    void Activate() override;

    ~URemovePlayerProxy();

    int32 player_id;
    
    UObject* worldContextObject;
    
private:
    void OnCompleted(bool success);
};
