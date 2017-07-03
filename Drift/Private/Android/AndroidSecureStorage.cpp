// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

#include "DriftPrivatePCH.h"

#include "AndroidSecureStorage.h"
#include "FileHelper.h"


#if PLATFORM_ANDROID


AndroidSecureStorage::AndroidSecureStorage(const FString& productName, const FString& serviceName)
: productName_{ productName }
, serviceName_{ serviceName }
{
}


bool AndroidSecureStorage::SaveValue(const FString& key, const FString& value, bool overwrite)
{
    auto fullPath = key + TEXT(".dat");
    uint32 flags = overwrite ? 0 : FILEWRITE_NoReplaceExisting;
    return FFileHelper::SaveStringToFile(value, *fullPath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), flags);
}


bool AndroidSecureStorage::GetValue(const FString& key, FString& value)
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


#endif // PLATFORM_ANDROID
