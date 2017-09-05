// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#pragma once

#include "DriftAPI.h"
#include "CommonDelegates.h"

#include "Kismet/BlueprintAsyncActionBase.h"

#include "SetPlayerNameProxy.generated.h"


class APlayerController;


USTRUCT(BlueprintType)
struct FKaleoSetPlayerNameResult
{
    GENERATED_BODY()

    UPROPERTY(BluePrintReadOnly, Category="Drift")
    int32 player_id;
};


UCLASS()
class USetPlayerNameProxy : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Drift", meta = (DisplayName = "Set Player Name", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
	static USetPlayerNameProxy* SetPlayerName(UObject* worldContextObject, APlayerController* playerController, FString name);

	UPROPERTY(BlueprintAssignable, Category = "Drift")
	FKaleoEmptySuccessDelegate OnSuccess;

	UPROPERTY(BlueprintAssignable, Category = "Drift")
	FKaleoEmptySuccessDelegate OnError;

	void Activate() override;

	USetPlayerNameProxy(const FObjectInitializer& oi);
	virtual ~USetPlayerNameProxy();

    FString name;
    
private:
	void OnCompleted(bool success);

    UObject* worldContextObject;
    TWeakObjectPtr<APlayerController> playerController;

    FDelegateHandle onPlayerNameSetHandle;
};
