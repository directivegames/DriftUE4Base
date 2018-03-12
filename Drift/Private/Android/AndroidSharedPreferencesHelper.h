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

#pragma once


#if PLATFORM_ANDROID
/**
* A C++ wrapper for the shared preferences system in Android:
* https://developer.android.com/reference/android/content/SharedPreferences.html
*/
class AndroidSharedPreferencesHelper
{
public:
	static void PutString(const FString& fileName, const FString& key, const FString& value);
	static FString GetString(const FString& fileName, const FString& key, const FString& defaultValue);
};

#endif // PLATFORM_ANDROID
