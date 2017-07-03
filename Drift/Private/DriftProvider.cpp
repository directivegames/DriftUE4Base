// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

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
    FName keyName = identifier == NAME_None ? DefaultInstanceName : identifier;

    FScopeLock lock{ &mutex };
    auto instance = instances.Find(keyName);
    if (instance == nullptr)
    {
        DriftBasePtr newInstance = MakeShareable(new FDriftBase(cache, keyName, instances.Num()));
        instances.Add(keyName, newInstance);
        instance = instances.Find(keyName);
    }
    return (*instance).Get();
}


void FDriftProvider::DestroyInstance(const FName& identifier)
{
    FName keyName = identifier == NAME_None ? DefaultInstanceName : identifier;
    
    if (auto instance = instances.Find(keyName))
    {
        if (instance->IsValid())
        {
            (*instance)->Shutdown();
        }

        FScopeLock lock{ &mutex };
        instances.Remove(keyName);
    }
}


void FDriftProvider::Close()
{
    FScopeLock lock{ &mutex };
	instances.Empty();
}
