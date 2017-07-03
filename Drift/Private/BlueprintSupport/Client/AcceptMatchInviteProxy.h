// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#pragma once

#include "OnlineBlueprintCallProxyBase.h"
#include "DriftAPI.h"
#include "DriftAPI.h"
#include "CommonDelegates.h"

#include "AcceptMatchInviteProxy.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAcceptMatchInviteDelegate, const FBlueprintMatchQueueStatus&, status);


UCLASS()
class UAcceptMatchInviteProxy : public UOnlineBlueprintCallProxyBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Drift|Matches", meta = (DisplayName = "Accept Match Invite", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static UAcceptMatchInviteProxy* AcceptMatchInvite(UObject* worldContextObject, const FBlueprintMatchInvite& invite);

    UPROPERTY(BlueprintAssignable)
    FAcceptMatchInviteDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FAcceptMatchInviteDelegate OnError;

    void Activate() override;

    UAcceptMatchInviteProxy(const FObjectInitializer& oi);
    virtual ~UAcceptMatchInviteProxy();

    FBlueprintMatchInvite invite;

private:
    void OnCompleted(bool success, const FMatchQueueStatus& status);

    UObject* worldContextObject;
};
