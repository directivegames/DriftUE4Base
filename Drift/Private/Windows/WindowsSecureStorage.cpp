// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

#include "DriftPrivatePCH.h"

#include "WindowsSecureStorage.h"

#include "Windows/AllowWindowsPlatformTypes.h"
	#include "wtypes.h"
#include "Windows/HideWindowsPlatformTypes.h"

#if PLATFORM_WINDOWS


WindowsSecureStorage::WindowsSecureStorage(const FString& productName, const FString& serviceName)
: productName_{ productName }
, serviceName_{ serviceName }
{
}


bool WindowsSecureStorage::SaveValue(const FString& key, const FString& value, bool overwrite)
{
    // TODO: Handle override
    HKEY hkey;
    FString path = FString::Printf(TEXT("SOFTWARE\\%s\\%s"), *serviceName_, *productName_);
    LSTATUS ret = RegCreateKeyEx(HKEY_CURRENT_USER, *path, 0L, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL);
    if (ret == ERROR_SUCCESS)
    {
        ret = RegSetValueEx(hkey, *key, NULL, REG_SZ, (LPBYTE)*value, value.Len() * sizeof(TCHAR));
        RegCloseKey(hkey);
        if (ret == ERROR_SUCCESS)
        {
            return true;
        }
    }
    return false;
}


bool WindowsSecureStorage::GetValue(const FString& key, FString& value)
{
    HKEY hkey;
    FString path = FString::Printf(TEXT("SOFTWARE\\%s\\%s"), *serviceName_, *productName_);
    if (RegOpenKeyEx(HKEY_CURRENT_USER, *path, NULL, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
    {
        ::uint32 type;
        TCHAR data[250];
        ::uint32 datalen = sizeof(data);
        LSTATUS ret = RegQueryValueEx(hkey, *key, NULL, (LPDWORD)&type, (LPBYTE)&data, (LPDWORD)&datalen);
        RegCloseKey(hkey);
        if (ret == ERROR_SUCCESS && type == REG_SZ)
        {
            value = data;
            return true;
        }
    }
    return false;
}


#endif // PLATFORM_WINDOWS
