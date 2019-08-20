// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#include "SecureStorageFactory.h"

#if PLATFORM_APPLE
#include "Apple/AppleSecureStorage.h"
#elif PLATFORM_WINDOWS
#include "Windows/WindowsSecureStorage.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxSecureStorage.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidSecureStorage.h"
#endif


TSharedPtr<ISecureStorage> SecureStorageFactory::GetSecureStorage(const FString& productName, const FString& serviceName)
{
#if PLATFORM_APPLE
	return MakeShareable(new AppleSecureStorage(productName, serviceName));
#elif PLATFORM_WINDOWS
	return MakeShareable(new WindowsSecureStorage(productName, serviceName));
#elif PLATFORM_LINUX
	return MakeShareable(new LinuxSecureStorage(productName, serviceName));
#elif PLATFORM_ANDROID
    return MakeShareable(new AndroidSecureStorage(productName, serviceName));
#else
	return nullptr;
#endif // Secure Storage
}
