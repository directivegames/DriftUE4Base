// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

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
    IDriftAPI* GetInstance(const FName& identifier, const FString& config) override;
    void DestroyInstance(const FName& identifier) override;
    void DestroyInstance(IDriftAPI* instance) override;

    void Close();

private:
    static FString MakeKey(const FName& identifier, const FString& config);

    TMap<FName, DriftApiPtr> instances;
    FCriticalSection mutex;
    
    TSharedPtr<IHttpCache> cache;
};
