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

#include "Containers/UnrealString.h"

#if PLATFORM_APPLE
class AppleUtility
{
public:
    static const FString& GetBundleVersion() { return bundleVersion; }
    static const FString& GetBundleShortVersion() { return bundleShortVersion; }
    static const FString& GetBundleName() { return bundleName; }
    
private:
    static const FString bundleVersion;
    static const FString bundleShortVersion;
    static const FString bundleName;

};
#endif

#if PLATFORM_IOS
class IOSUtility
{
public:
    static const FString& GetIOSVersion() { return iOSVersion; }
    static const FString& GetIOSBuild();
    static const FString& GetHardwareModel();
    
private:
    static const FString iOSVersion;
    static FString iOSBuild;
    static FString hardwareModel;
};
#endif
