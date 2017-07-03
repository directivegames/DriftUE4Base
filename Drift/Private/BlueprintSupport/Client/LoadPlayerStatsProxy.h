
#pragma once

#include "CommonDelegates.h"

#include "Kismet/BlueprintAsyncActionBase.h"

#include "LoadPlayerStatsProxy.generated.h"


UCLASS()
class ULoadPlayerStatsProxy : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
    
public:
    UFUNCTION(BlueprintCallable, Category = "Drift|Metrics", meta = (DisplayName = "Load Player Stats", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static ULoadPlayerStatsProxy* LoadPlayerStats(UObject* worldContextObject);
    
    UPROPERTY(BlueprintAssignable)
    FKaleoEmptySuccessDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FKaleoEmptySuccessDelegate OnError;
    
    void Activate() override;
    
    ULoadPlayerStatsProxy(const FObjectInitializer& oi);
    ~ULoadPlayerStatsProxy();
    
private:
    void OnCompleted(bool success);

    UObject* worldContextObject;
    
    FDelegateHandle onPlayerStatsLoadedHandle;
};
