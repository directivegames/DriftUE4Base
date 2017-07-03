
#pragma once

#include "CommonDelegates.h"
#include "DriftAPI.h"

#include "Kismet/BlueprintAsyncActionBase.h"

#include "LoadFriendsListProxy.generated.h"


UCLASS()
class ULoadFriendsListProxy : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
    
public:
    UFUNCTION(BlueprintCallable, Category = "Drift|Friends", meta = (DisplayName = "Load Friends List", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static ULoadFriendsListProxy* LoadFriendsList(UObject* worldContextObject);

    UPROPERTY(BlueprintAssignable)
    FKaleoEmptySuccessDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FKaleoEmptySuccessDelegate OnError;

    void Activate() override;

    ULoadFriendsListProxy(const FObjectInitializer& oi);
    ~ULoadFriendsListProxy();
    
    FString name;
    
private:
    void OnCompleted(bool result);

    UObject* worldContextObject;
};
