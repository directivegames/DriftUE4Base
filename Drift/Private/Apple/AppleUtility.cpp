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

#include "AppleUtility.h"
#include "DriftPrivatePCH.h"


#if PLATFORM_IOS
#include <sys/sysctl.h>
#endif // PLATFORM_IOS

#if PLATFORM_APPLE

const FString AppleUtility::bundleVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"];
const FString AppleUtility::bundleShortVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
const FString AppleUtility::bundleName = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];

#endif //PLATFORM_APPLE

#if PLATFORM_IOS

const FString IOSUtility::iOSVersion = [[UIDevice currentDevice] systemVersion];
FString IOSUtility::iOSBuild;
FString IOSUtility::hardwareModel;

const FString& IOSUtility::GetIOSBuild()
{
    if (iOSBuild.IsEmpty())
    {
        char buffer[PATH_MAX] = {};
        size_t bufferSize = PATH_MAX;
        sysctlbyname("kern.osversion", buffer, &bufferSize, nullptr, 0);
        iOSBuild = ANSI_TO_TCHAR(buffer);
    }
    return iOSBuild;
}

const FString& IOSUtility::GetHardwareModel()
{
    if (hardwareModel.IsEmpty())
    {
        char buffer[PATH_MAX] = {};
        size_t bufferSize = PATH_MAX;
        sysctlbyname("hw.machine", buffer, &bufferSize, nullptr, 0);
        hardwareModel = ANSI_TO_TCHAR(buffer);
    }
    return hardwareModel;
}

#endif // PLATFORM_IOS
