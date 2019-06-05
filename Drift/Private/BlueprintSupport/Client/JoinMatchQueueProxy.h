// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#pragma once

#include "Net/OnlineBlueprintCallProxyBase.h"
#include "DriftAPI.h"
#include "DriftAPI.h"
#include "CommonDelegates.h"

#include "JoinMatchQueueProxy.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJoinMatchQueueDelegate, const FBlueprintMatchQueueStatus&, status);


UCLASS()
class UJoinMatchQueueProxy : public UOnlineBlueprintCallProxyBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Drift|Matches", meta = (DisplayName = "Join Match Queue", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static UJoinMatchQueueProxy* JoinMatchQueue(UObject* worldContextObject);

    UPROPERTY(BlueprintAssignable)
    FJoinMatchQueueDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FJoinMatchQueueDelegate OnError;

    void Activate() override;

    UJoinMatchQueueProxy(const FObjectInitializer& oi);
    virtual ~UJoinMatchQueueProxy();

private:
    void OnCompleted(bool success, const FMatchQueueStatus& status);

    UObject* worldContextObject;
};
