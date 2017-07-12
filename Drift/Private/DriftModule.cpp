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

#include "DriftPrivatePCH.h"
#include "DriftModule.h"
#include "DriftBase.h"

#include "Features/IModularFeatures.h"


#define LOCTEXT_NAMESPACE "Drift"


IMPLEMENT_MODULE(FDriftModule, Drift)


FDriftModule::FDriftModule()
{

}


void FDriftModule::StartupModule()
{
    IModularFeatures::Get().RegisterModularFeature(TEXT("Drift"), &provider);
}


void FDriftModule::ShutdownModule()
{
    IModularFeatures::Get().UnregisterModularFeature(TEXT("Drift"), &provider);
}


#undef LOCTEXT_NAMESPACE
