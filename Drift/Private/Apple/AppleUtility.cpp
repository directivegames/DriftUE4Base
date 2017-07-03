// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

#include "DriftPrivatePCH.h"

#include "AppleUtility.h"

#if PLATFORM_APPLE

const FString AppleUtility::bundleVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"];
const FString AppleUtility::bundleShortVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"];
const FString AppleUtility::bundleName = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];

#endif //PLATFORM_APPLE

#if PLATFORM_IOS
const FString IOSUtility::iOSVersion = [[UIDevice currentDevice] systemVersion];
#endif
