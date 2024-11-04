/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2016-2019 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/

#include "WindowsSecureStorage.h"

#include "Windows/AllowWindowsPlatformTypes.h"
	#include "wtypes.h"
    #include <wincred.h>
    #include <string>
#include "Windows/HideWindowsPlatformTypes.h"

#if PLATFORM_WINDOWS

#define WITH_CREDENTIAL_MANAGER 1

WindowsSecureStorage::WindowsSecureStorage(const FString& productName, const FString& serviceName)
: productName_{ productName }
, serviceName_{ serviceName }
{
}


FString WindowsSecureStorage::MakeUniqueKey(const FString& key)
{
    return FString::Printf(TEXT("%s.%s.%s"), *serviceName_, *productName_, *key);
}


bool WindowsSecureStorage::SaveValue(const FString& key, const FString& value, bool overwrite)
{
#if WITH_CREDENTIAL_MANAGER
    const auto UniqueKey = MakeUniqueKey(key);
    CREDENTIAL Credential = { 0 };
    Credential.Type = CRED_TYPE_GENERIC;
    Credential.TargetName = const_cast<LPWSTR>(*UniqueKey);
    Credential.CredentialBlob = (LPBYTE)*value;
    Credential.CredentialBlobSize = (DWORD)value.Len() * sizeof(TCHAR);
    Credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
    return CredWrite(&Credential, 0);
#else
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
#endif
}


bool WindowsSecureStorage::GetValue(const FString& key, FString& value)
{
#if WITH_CREDENTIAL_MANAGER
    const auto UniqueKey = MakeUniqueKey(key);
    PCREDENTIAL pcred = NULL;
    if (CredRead(*UniqueKey, CRED_TYPE_GENERIC, 0, &pcred))
    {
        std::wstring secret;
        secret.assign((TCHAR*)pcred->CredentialBlob, pcred->CredentialBlobSize / sizeof(TCHAR));
        value = secret.c_str();
        CredFree(pcred);
        return true;
    }
    return false;
#else
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
#endif
}


#endif // PLATFORM_WINDOWS
