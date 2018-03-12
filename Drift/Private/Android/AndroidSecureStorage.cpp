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

#include "DriftPrivatePCH.h"

#include "AndroidSecureStorage.h"
#include "AndroidSharedPreferencesHelper.h"


#if PLATFORM_ANDROID


AndroidSecureStorage::AndroidSecureStorage(const FString& productName, const FString& serviceName)
: productName_{ productName }
, serviceName_{ serviceName }
{
}


bool AndroidSecureStorage::SaveValue(const FString& key, const FString& value, bool overwrite)
{
	if (!overwrite)
	{
		FString existing;
		if (GetValue(key, existing))
		{
			return true;
		}
	}

	AndroidSharedPreferencesHelper::PutString(TEXT("test"), key, value);
	return true;
}


bool AndroidSecureStorage::GetValue(const FString& key, FString& value)
{    
	auto temp = AndroidSharedPreferencesHelper::GetString(TEXT("test"), key, TEXT(""));
	if (temp.Len())
	{
		value = temp;
		return true;
	}

	return false;
}


#endif // PLATFORM_ANDROID
