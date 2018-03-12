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

#include "AndroidSharedPreferencesHelper.h"

#if PLATFORM_ANDROID
#include "AndroidJNI.h"
#include "AndroidApplication.h"
#endif


#if PLATFORM_ANDROID


void AndroidSharedPreferencesHelper::PutString(const FString& fileName, const FString& key, const FString& value)
{
	if (auto env = FAndroidApplication::GetJavaEnv())
	{
		static auto method = FJavaWrapper::FindMethod(env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_SharedPreferencesPutString", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V", false);
		auto fileNameJava = env->NewStringUTF(TCHAR_TO_UTF8(*fileName));
		auto keyJava = env->NewStringUTF(TCHAR_TO_UTF8(*key));
		auto valueJava = env->NewStringUTF(TCHAR_TO_UTF8(*value));
		FJavaWrapper::CallVoidMethod(env, FJavaWrapper::GameActivityThis, method, fileNameJava, keyJava, valueJava);
		env->DeleteLocalRef(fileNameJava);
		env->DeleteLocalRef(keyJava);
		env->DeleteLocalRef(valueJava);
	}
}


FString AndroidSharedPreferencesHelper::GetString(const FString& fileName, const FString& key, const FString& defaultValue)
{
	FString value;
	if (auto env = FAndroidApplication::GetJavaEnv())
	{
		static auto method = FJavaWrapper::FindMethod(env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_SharedPreferencesGetString", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", false);
		auto fileNameJava = env->NewStringUTF(TCHAR_TO_UTF8(*fileName));
		auto keyJava = env->NewStringUTF(TCHAR_TO_UTF8(*key));
		auto defaultValueJava = env->NewStringUTF(TCHAR_TO_UTF8(*defaultValue));
		auto valueJava = (jstring)FJavaWrapper::CallObjectMethod(env, FJavaWrapper::GameActivityThis, method, fileNameJava, keyJava, defaultValueJava);
		if (valueJava)
		{
			auto javaChars = env->GetStringUTFChars(valueJava, 0);
			value = FString(UTF8_TO_TCHAR(javaChars));
			env->ReleaseStringUTFChars(valueJava, javaChars);
			env->DeleteLocalRef(valueJava);
		}
		env->DeleteLocalRef(fileNameJava);
		env->DeleteLocalRef(keyJava);
		env->DeleteLocalRef(defaultValueJava);
	}

	return value;
}


#endif // PLATFORM_ANDROID
