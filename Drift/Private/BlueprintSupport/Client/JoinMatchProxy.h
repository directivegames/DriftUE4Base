// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#pragma once

#include "Net/OnlineBlueprintCallProxyBase.h"
#include "DriftAPI.h"
#include "DriftAPI.h"
#include "CommonDelegates.h"

#include "JoinMatchProxy.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJoinMatchDelegate, const FBlueprintMatchQueueStatus&, status);


UCLASS()
class UJoinMatchProxy : public UOnlineBlueprintCallProxyBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Drift|Matches", meta = (DisplayName = "Join Match", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static UJoinMatchProxy* JoinMatch(UObject* worldContextObject, const FBlueprintMatchInvite& invite);

    UPROPERTY(BlueprintAssignable)
    FJoinMatchDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FJoinMatchDelegate OnError;

    void Activate() override;

    UJoinMatchProxy(const FObjectInitializer& oi);
    virtual ~UJoinMatchProxy();

    FBlueprintMatchInvite invite;

private:
    void OnCompleted(bool success, const FMatchQueueStatus& status);

    UObject* worldContextObject;
};
