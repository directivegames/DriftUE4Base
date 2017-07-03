// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#pragma once

#include "DriftAPI.h"
#include "CommonDelegates.h"

#include "Kismet/BlueprintAsyncActionBase.h"

#include "LoadPlayerAvatarUrlProxy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLoadAvatarUrlDelegate, FString, avatarUrl);

UCLASS()
class ULoadPlayerAvatarUrlProxy : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Drift", meta = (DisplayName = "Load Player Avatar Url", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
	static ULoadPlayerAvatarUrlProxy* LoadPlayerAvatarUrl(UObject* worldContextObject);

	UPROPERTY(BlueprintAssignable, Category = "Drift")
	FLoadAvatarUrlDelegate OnSuccess;

	UPROPERTY(BlueprintAssignable, Category = "Drift")
	FLoadAvatarUrlDelegate OnError;	

	void Activate() override;

private:
	void OnCompleted(const FString& url);

    UObject* worldContextObject = nullptr;
};
