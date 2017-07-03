// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

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
