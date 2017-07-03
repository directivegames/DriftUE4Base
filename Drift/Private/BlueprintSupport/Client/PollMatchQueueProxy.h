// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#pragma once

#include "OnlineBlueprintCallProxyBase.h"
#include "DriftAPI.h"
#include "CommonDelegates.h"

#include "PollMatchQueueProxy.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPollMatchQueueDelegate, const FBlueprintMatchQueueStatus&, queue);


UCLASS()
class UPollMatchQueueProxy : public UOnlineBlueprintCallProxyBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Drift|Matches", meta = (DisplayName = "Poll Match Queue", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static UPollMatchQueueProxy* PollMatchQueue(UObject* worldContextObject);

    UPROPERTY(BlueprintAssignable)
    FPollMatchQueueDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FPollMatchQueueDelegate OnError;

    void Activate() override;

    UPollMatchQueueProxy(const FObjectInitializer& oi);
    virtual ~UPollMatchQueueProxy();

private:
    void OnCompleted(bool success, const FMatchQueueStatus& match);

    UObject* worldContextObject;
};
