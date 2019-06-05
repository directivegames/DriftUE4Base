// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"


class FDriftHttpModule : public IModuleInterface
{
public:
    FDriftHttpModule();
    
    bool IsGameModule() const override
    {
        return true;
    }

    void StartupModule() override;
    void ShutdownModule() override;
};
