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

#include "IDriftProvider.h"
#include "DriftAPI.h"
#include "HttpCache.h"

#include "Features/IModularFeature.h"


class FDriftProvider : public IDriftProvider
{
public:
    FDriftProvider();
    virtual ~FDriftProvider() {}

    IDriftAPI* GetInstance(const FName& identifier) override;
    void DestroyInstance(const FName& identifier) override;

    void Close();

private:
    TMap<FName, DriftApiPtr> instances;
    FCriticalSection mutex;
    
    TSharedPtr<IHttpCache> cache;
};
