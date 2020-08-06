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

#include "HoloLensSecureStorage.h"

#include "HoloLens/AllowWindowsPlatformTypes.h"

#if PLATFORM_HOLOLENS

HoloLensSecureStorage::HoloLensSecureStorage(const FString& productName, const FString& serviceName)
: productName_{ productName }
, serviceName_{ serviceName }
{
}

bool HoloLensSecureStorage::SaveValue(const FString& key, const FString& value, bool overwrite)
{
	const auto path = FString::Printf(TEXT("%s\\%s"), *serviceName_, *productName_);

	if (auto LocalSettings = Windows::Storage::ApplicationData::Current->LocalSettings)
	{
		auto PlatformPath = ref new Platform::String(*path);
		if (auto Container = LocalSettings->CreateContainer(PlatformPath, Windows::Storage::ApplicationDataCreateDisposition::Always))
		{
			auto PlatformKey = ref new Platform::String(*key);
			auto PlatformValue = ref new Platform::String(*value);
			Container->Values->Insert(PlatformKey, PlatformValue);
			return true;
		}
	}

    return false;
}


bool HoloLensSecureStorage::GetValue(const FString& key, FString& value)
{
	const auto path = FString::Printf(TEXT("%s\\%s"), *serviceName_, *productName_);

	if (auto LocalSettings = Windows::Storage::ApplicationData::Current->LocalSettings)
	{
		auto PlatformPath = ref new Platform::String(*path);
		if (auto Container = LocalSettings->CreateContainer(PlatformPath, Windows::Storage::ApplicationDataCreateDisposition::Always))
		{
			auto PlatformKey = ref new Platform::String(*key);
			if (Container->Values->HasKey(PlatformKey))
			{
				if (auto PlatformValue = dynamic_cast<Platform::String^>(Container->Values->Lookup(PlatformKey)))
				{
					static TCHAR Result[PLATFORM_MAX_FILEPATH_LENGTH] = TEXT("");
					FCString::Strncpy(Result, PlatformValue->Data(), PLATFORM_MAX_FILEPATH_LENGTH);
					value = FString(Result);
					return true;
				}
			}
		}
	}

    return false;
}

#endif // PLATFORM_WINDOWS

#include "HoloLens/HideWindowsPlatformTypes.h"
