
#pragma once

#include "CommonDelegates.h"

#include "Kismet/BlueprintAsyncActionBase.h"

#include "SavePlayerGameStateProxy.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerGameStateSavedDelegate);


UCLASS()
class USavePlayerGameStateProxy : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
    
public:
    UFUNCTION(BlueprintCallable, Category = "Drift|Game State", meta = (DisplayName = "Save Player Game State", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static USavePlayerGameStateProxy* LoadStaticData(UObject* worldContextObject, FString name, FString state);
    
    UPROPERTY(BlueprintAssignable)
    FPlayerGameStateSavedDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FPlayerGameStateSavedDelegate OnError;
    
    void Activate() override;
    
    USavePlayerGameStateProxy(const FObjectInitializer& oi);
    ~USavePlayerGameStateProxy();
    
    FString name_;
    FString state_;

private:
    void OnCompleted(bool success, const FString& name);

    UObject* worldContextObject;
};
