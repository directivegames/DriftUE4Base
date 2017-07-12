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

#pragma once

#include "ModuleManager.h"
#include "../Public/IErrorReporter.h"


class FErrorReporterModule : public IModuleInterface
{
public:
    FErrorReporterModule();
    
    bool IsGameModule() const override
    {
        return true;
    }

    void StartupModule() override;
    void ShutdownModule() override;

    IErrorReporter* GetErrorReporter()
    {
        return instance_.Get();
    }

    void SetErrorReporter(TUniquePtr<IErrorReporter> instance)
    {
        instance_ = MoveTemp(instance);
    }

private:
    TUniquePtr<IErrorReporter> instance_;
};
