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
