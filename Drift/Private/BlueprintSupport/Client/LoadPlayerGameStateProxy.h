
#pragma once

#include "CommonDelegates.h"
#include "DriftAPI.h"

#include "Kismet/BlueprintAsyncActionBase.h"

#include "LoadPlayerGameStateProxy.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerGameStateLoadedDelegate, ELoadPlayerGameStateResult, error, const FString&, state);


UCLASS()
class ULoadPlayerGameStateProxy : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
    
public:
    UFUNCTION(BlueprintCallable, Category = "Drift|Game State", meta = (DisplayName = "Load Player Game State", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static ULoadPlayerGameStateProxy* LoadPlayerGameState(UObject* worldContextObject, FString name);
    
    UPROPERTY(BlueprintAssignable)
    FPlayerGameStateLoadedDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FPlayerGameStateLoadedDelegate OnError;
    
    void Activate() override;
    
    ULoadPlayerGameStateProxy(const FObjectInitializer& oi);
    ~ULoadPlayerGameStateProxy();
    
    FString name_;
    
private:
    void OnCompleted(ELoadPlayerGameStateResult result, const FString& name, const FString& state);

    UObject* worldContextObject_;
};
