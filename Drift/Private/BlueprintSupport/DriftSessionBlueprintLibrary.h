// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#pragma once

#include "DriftSessionBlueprintLibrary.generated.h"


class APlayerController;


UCLASS()
class UDriftSessionBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    /** Disconnect and shutdown all sessions */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "worldContextObject"), Category = "Drift|Session")
	static void Disconnect(UObject* worldContextObject, APlayerController* playerController);
};
