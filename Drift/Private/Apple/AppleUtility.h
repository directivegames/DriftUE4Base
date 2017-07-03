// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "UnrealString.h"

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
    
private:
    static const FString iOSVersion;
};
#endif
