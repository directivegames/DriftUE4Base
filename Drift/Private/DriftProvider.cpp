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

#include "DriftProvider.h"
#include "DriftBase.h"

#include "FileHttpCacheFactory.h"


const FName DefaultInstanceName = TEXT("DefaultInstance");


FDriftProvider::FDriftProvider()
: cache{ FileHttpCacheFactory().Create() }
{
}


IDriftAPI* FDriftProvider::GetInstance(const FName& identifier)
{
    const FName keyName = identifier == NAME_None ? DefaultInstanceName : identifier;

    FScopeLock lock{ &mutex };
    auto instance = instances.Find(keyName);
    if (instance == nullptr)
    {
        const DriftBasePtr newInstance = MakeShareable(new FDriftBase(cache, keyName, instances.Num()), [](IDriftAPI* _instance)
        {
			_instance->Shutdown();
            delete _instance;
        });
        instances.Add(keyName, newInstance);
        instance = instances.Find(keyName);
    }
    return (*instance).Get();
}


void FDriftProvider::DestroyInstance(const FName& identifier)
{
    const FName keyName = identifier == NAME_None ? DefaultInstanceName : identifier;
    
    if (const auto instance = instances.Find(keyName))
    {
        FScopeLock lock{ &mutex };
        instances.Remove(keyName);
    }
}


void FDriftProvider::DestroyInstance(IDriftAPI* instance)
{
    if (instance == nullptr)
    {
        return;
    }

    /**
     * We must pass in a shared pointer, but we don't have the original one,
     * so create a temporary which doesn't delete the pointer afterwards.
     */
    if (const auto key = instances.FindKey(MakeShareable(instance, [](IDriftAPI*) {})))
    {
        DestroyInstance(*key);
    }
}


void FDriftProvider::Close()
{
    FScopeLock lock{ &mutex };
	instances.Empty();
}
