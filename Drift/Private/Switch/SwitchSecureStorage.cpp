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

#include "SwitchSecureStorage.h"

#include "DriftPrivatePCH.h"
#include "Misc/FileHelper.h"

#if PLATFORM_SWITCH


SwitchSecureStorage::SwitchSecureStorage(const FString& productName, const FString& serviceName)
: productName_{ productName }
, serviceName_{ serviceName }
{
}


bool SwitchSecureStorage::SaveValue(const FString& key, const FString& value, bool overwrite)
{
	auto fullPath = key + TEXT(".dat");
	uint32 flags = overwrite ? 0 : FILEWRITE_NoReplaceExisting;
	return FFileHelper::SaveStringToFile(value, *fullPath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), flags);
}


bool SwitchSecureStorage::GetValue(const FString& key, FString& value)
{
	auto fullPath = key + TEXT(".dat");
	FString fileContent;
	if (FFileHelper::LoadFileToString(fileContent, *fullPath))
	{
		value = fileContent;
		return true;
	}
	return false;
}


#endif // PLATFORM_SWITCH
