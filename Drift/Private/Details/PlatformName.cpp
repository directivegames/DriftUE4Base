
#include "DriftPrivatePCH.h"

#include "PlatformMisc.h"
#include "PlatformName.h"


namespace details
{

const TCHAR* GetPlatformName()
{
#if PLATFORM_WINDOWS
    return L"Windows";
#elif PLATFORM_PS4
    return L"PS4";
#elif PLATFORM_XBOXONE
    return L"XBOX1";
#elif PLATFORM_MAC
    return L"MAC";
#elif PLATFORM_IOS
    return FIOSPlatformMisc::GetDefaultDeviceProfileName();
#elif PLATFORM_ANDROID
    return L"Android";
#elif PLATFORM_WINRT_ARM
    return L"WinRT";
#elif PLATFORM_WINRT
    return L"WinRT";
#elif PLATFORM_HTML5
    return L"HTML5";
#elif PLATFORM_LINUX
    return L"Linux";
#else
#error Unknown Platform
#endif
}

} // namespace details
