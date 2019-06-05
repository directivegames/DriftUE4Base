// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#include "DriftPrivatePCH.h"

#include "LinuxSecureStorage.h"


#if PLATFORM_LINUX


LinuxSecureStorage::LinuxSecureStorage(const FString& productName, const FString& serviceName)
: productName_{ productName }
, serviceName_{ serviceName }
{
}


bool LinuxSecureStorage::SaveValue(const FString& key, const FString& value, bool overwrite)
{
    return false;
}


bool LinuxSecureStorage::GetValue(const FString& key, FString& value)
{
    return false;
}


#endif // PLATFORM_LINUX
