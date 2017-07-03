// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#pragma once

#include "DriftAPI.h"
#include "CommonDelegates.h"

#include "Kismet/BlueprintAsyncActionBase.h"

#include "AuthenticatePlayerProxy.generated.h"


class APlayerController;


USTRUCT()
struct FDriftAuthenticatePlayerResult
{
    GENERATED_BODY()

public:
    UPROPERTY(BluePrintReadOnly, Category="Drift")
    int32 playerId;

    UPROPERTY(BluePrintReadOnly, Category="Drift")
    FString playerName;

    UPROPERTY(BluePrintReadOnly, Category="Drift")
    EAuthenticationResult result;
    
    UPROPERTY(BluePrintReadOnly, Category="Drift")
    FString error;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKaleoAuthenticatePlayerResultDelegate, const FDriftAuthenticatePlayerResult&, result);


UCLASS()
class UAuthenticatePlayerProxy : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Drift", meta = (DisplayName = "Authenticate Player", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
	static UAuthenticatePlayerProxy* AuthenticatePlayer(UObject* worldContextObject, APlayerController* playerController);

	UPROPERTY(BlueprintAssignable, Category = "Drift")
	FKaleoAuthenticatePlayerResultDelegate OnSuccess;

	UPROPERTY(BlueprintAssignable, Category = "Drift")
	FKaleoAuthenticatePlayerResultDelegate OnError;

	void Activate() override;

	UAuthenticatePlayerProxy(const FObjectInitializer& oi);
	virtual ~UAuthenticatePlayerProxy();

private:
	void OnCompleted(bool success, const FPlayerAuthenticatedInfo& info);

    UObject* worldContextObject;
    TWeakObjectPtr<APlayerController> playerController;

    FDelegateHandle onPlayerAuthenticatedHandle;
};
