// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#pragma once

#include "OnlineBlueprintCallProxyBase.h"
#include "CommonDelegates.h"

#include "UpdateMatchProxy.generated.h"


UCLASS()
class UUpdateMatchProxy : public UOnlineBlueprintCallProxyBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FKaleoEmptySuccessDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable, VisibleAnywhere)
    FKaleoErrorDelegate OnError;

    UFUNCTION(BlueprintCallable, Category = "Drift", meta = (DisplayName = "Update Match", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static UUpdateMatchProxy* UpdateMatch(UObject* worldContextObject, FString match_url, FString status);

    void Activate() override;

    ~UUpdateMatchProxy();

    FString status;
    
private:
    UObject* worldContextObject;
};
