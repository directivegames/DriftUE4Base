// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#include "DriftPrivatePCH.h"

#include "DriftSessionBlueprintLibrary.h"
#include "DriftUtils.h"


void UDriftSessionBlueprintLibrary::Disconnect(UObject* worldContextObject, APlayerController* playerController)
{
	FDriftWorldHelper helper{ worldContextObject };
	helper.DestroyInstance();
}
