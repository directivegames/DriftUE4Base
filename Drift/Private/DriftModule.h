// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "DriftAPI.h"
#include "DriftProvider.h"


class FDriftModule : public IModuleInterface
{
public:
    FDriftModule();
    
    bool IsGameModule() const override
    {
        return true;
    }

    void StartupModule() override;
    void ShutdownModule() override;

private:
    FDriftProvider provider;
};
