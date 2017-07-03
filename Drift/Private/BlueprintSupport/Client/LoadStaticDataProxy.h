
#pragma once

#include "CommonDelegates.h"

#include "Kismet/BlueprintAsyncActionBase.h"

#include "LoadStaticDataProxy.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStaticDataLoadedDelegate, const FString&, data);


UCLASS()
class ULoadStaticDataProxy : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
    
public:
    UFUNCTION(BlueprintCallable, Category = "Drift|Static Data", meta = (DisplayName = "Load Static Data", BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"))
    static ULoadStaticDataProxy* LoadStaticData(UObject* worldContextObject, FString name, FString ref);
    
    UPROPERTY(BlueprintAssignable)
    FStaticDataLoadedDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FStaticDataLoadedDelegate OnError;
    
    void Activate() override;
    
    ULoadStaticDataProxy(const FObjectInitializer& oi);
    ~ULoadStaticDataProxy();
    
    FString name;
    FString ref;
    
private:
    void OnProgressInternal(const FString& name, int32 progress, int32 total);
    void OnCompleted(bool success, const FString& data);

    UObject* worldContextObject;
    
    FDelegateHandle onStaticDataLoadedHandle;
};
