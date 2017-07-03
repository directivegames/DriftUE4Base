// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#pragma once

#include "OnlineBlueprintCallProxyBase.h"
#include "DriftAPI.h"
#include "DriftAPI.h"
#include "CommonDelegates.h"

#include "LeaveMatchQueueProxy.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLeaveMatchQueueDelegate, bool, result);


UCLASS()
class ULeaveMatchQueueProxy : public UOnlineBlueprintCallProxyBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Drift|Matches", meta = (DisplayName = "Leave Match Queue", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static ULeaveMatchQueueProxy* LeaveMatchQueue(UObject* worldContextObject);

    UPROPERTY(BlueprintAssignable)
    FLeaveMatchQueueDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FLeaveMatchQueueDelegate OnError;

    void Activate() override;

    ULeaveMatchQueueProxy(const FObjectInitializer& oi);
    virtual ~ULeaveMatchQueueProxy();

private:
    void OnCompleted(bool success);

    UObject* worldContextObject;
};
