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
#elif PLATFORM_HOLOLENS
#include "HoloLens/HoloLensSecureStorage.h"
#elif PLATFORM_SWITCH
#include "Switch/SwitchSecureStorage.h"
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
#elif PLATFORM_HOLOLENS
	return MakeShareable(new HoloLensSecureStorage(productName, serviceName));
#elif PLATFORM_SWITCH
	return MakeShareable(new SwitchSecureStorage(productName, serviceName));
#else
	ensureMsgf(false, TEXT("Missing secure storage support!"));
	return nullptr;
#endif // Secure Storage
}
