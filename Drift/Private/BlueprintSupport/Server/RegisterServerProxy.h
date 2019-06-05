// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#pragma once

#include "Net/OnlineBlueprintCallProxyBase.h"
#include "DriftAPI.h"
#include "CommonDelegates.h"

#include "RegisterServerProxy.generated.h"


UCLASS()
class URegisterServerProxy : public UOnlineBlueprintCallProxyBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Drift", meta = (DisplayName = "Register Server", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static URegisterServerProxy* RegisterServer(UObject* worldContextObject);

    UPROPERTY(BlueprintAssignable)
    FKaleoEmptySuccessDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FKaleoErrorDelegate OnError;

    void Activate() override;

    ~URegisterServerProxy();

private:
    void OnCompleted(bool success);
    
    UObject* worldContextObject;

    FDelegateHandle onSeverRegisteredHandle;
};
