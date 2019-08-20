
#include "PlatformName.h"
#include "PlatformMisc.h"


namespace details
{

const TCHAR* GetPlatformName()
{
#if PLATFORM_WINDOWS
    return TEXT("Windows");
#elif PLATFORM_PS4
    return TEXT("PS4");
#elif PLATFORM_XBOXONE
    return TEXT("XBOX1");
#elif PLATFORM_MAC
    return TEXT("MAC");
#elif PLATFORM_IOS
    return FIOSPlatformMisc::GetDefaultDeviceProfileName();
#elif PLATFORM_ANDROID
    return TEXT("Android");
#elif PLATFORM_WINRT_ARM
    return TEXT("WinRT");
#elif PLATFORM_WINRT
    return TEXT("WinRT");
#elif PLATFORM_HTML5
    return TEXT("HTML5");
#elif PLATFORM_LINUX
    return TEXT("Linux");
#else
#error Unknown Platform
#endif
}

} // namespace details
