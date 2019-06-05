// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

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
    return GetInstance(identifier, TEXT(""));
}


IDriftAPI* FDriftProvider::GetInstance(const FName& identifier, const FString& config)
{
    const FName keyName{ *MakeKey(identifier, config) };

    FScopeLock lock{ &mutex };
    auto instance = instances.Find(keyName);
    if (instance == nullptr)
    {
        const DriftBasePtr newInstance = MakeShareable(new FDriftBase(cache, keyName, instances.Num(), config), [](IDriftAPI* drift)
        {
			drift->Shutdown();
            delete drift;
        });
        instances.Add(keyName, newInstance);
        instance = instances.Find(keyName);
    }
    return (*instance).Get();
}


void FDriftProvider::DestroyInstance(const FName& identifier)
{
    const FName keyName{ *MakeKey(identifier, TEXT("")) };
    
    FScopeLock lock{ &mutex };
    instances.Remove(keyName);
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
        FScopeLock lock{ &mutex };
        instances.Remove(*key);
    }
}


void FDriftProvider::Close()
{
    FScopeLock lock{ &mutex };
	instances.Empty();
}


FString FDriftProvider::MakeKey(const FName& identifier, const FString& config)
{
    return FString::Printf(TEXT("%s.%s"),
        *(identifier == NAME_None ? DefaultInstanceName : identifier).ToString(),
        config.IsEmpty() ? TEXT("default") : *config
    );
}
