// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

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
