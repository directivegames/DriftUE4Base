// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#include "DriftPrivatePCH.h"

#include "AppleUtility.h"

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
