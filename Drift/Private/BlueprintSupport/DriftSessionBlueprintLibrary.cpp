/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2016-2017 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/

#include "DriftSessionBlueprintLibrary.h"
#include "DriftUtils.h"


void UDriftSessionBlueprintLibrary::Disconnect(UObject* worldContextObject, APlayerController* playerController)
{
	FDriftWorldHelper helper{ worldContextObject };
	helper.DestroyInstance();
}
