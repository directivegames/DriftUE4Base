// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "ModuleManager.h"


class FRapidJsonModule : public IModuleInterface
{
public:
    FRapidJsonModule();
    
    bool IsGameModule() const override
    {
        return true;
    }

    void StartupModule() override;
    void ShutdownModule() override;
};
