// Copyright (C) 2024-2025 - Directive Games Limited - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "DriftUtilityFunctions.generated.h"


UCLASS()
class DRIFT_API UDriftUtilityFunctions : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Drift")
    static TArray<FString> GetGameVersionStringSplit();

    UFUNCTION(BlueprintCallable, Category="Drift")
    static FString GetGameVersionString();
};
