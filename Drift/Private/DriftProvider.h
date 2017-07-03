// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "IDriftProvider.h"
#include "DriftAPI.h"
#include "HttpCache.h"

#include "Features/IModularFeature.h"


class FDriftProvider : public IDriftProvider
{
public:
    FDriftProvider();

    IDriftAPI* GetInstance(const FName& identifier) override;
    void DestroyInstance(const FName& identifier) override;

    void Close();

private:
    TMap<FName, DriftApiPtr> instances;
    FCriticalSection mutex;
    
    TSharedPtr<IHttpCache> cache;
};
