// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#pragma once

#include "OnlineBlueprintCallProxyBase.h"
#include "DriftAPI.h"
#include "DriftAPI.h"
#include "CommonDelegates.h"

#include "SendMatchInviteProxy.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSendMatchInviteDelegate, const FBlueprintMatchQueueStatus&, status);


UCLASS()
class USendMatchInviteProxy : public UOnlineBlueprintCallProxyBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Drift|Matches", meta = (DisplayName = "Send Match Invite", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static USendMatchInviteProxy* SendMatchInvite(UObject* worldContextObject, int32 playerID);

    UPROPERTY(BlueprintAssignable)
    FSendMatchInviteDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FSendMatchInviteDelegate OnError;

    void Activate() override;

    USendMatchInviteProxy(const FObjectInitializer& oi);
    virtual ~USendMatchInviteProxy();

    int32 playerID;

private:
    void OnCompleted(bool success, const FMatchQueueStatus& status);

    UObject* worldContextObject;
};
