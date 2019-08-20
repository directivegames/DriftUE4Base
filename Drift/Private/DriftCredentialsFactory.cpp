// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#include "DriftCredentialsFactory.h"
#include "Details/PlatformName.h"


void FDriftCredentialsFactory::MakeUniqueCredentials(FString& id, FString& password)
{
    FString guidString = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    FString platformName = details::GetPlatformName();
    id = FString::Printf(TEXT("%s-%s"), *platformName, *guidString);
    password = FGuid::NewGuid().ToString(EGuidFormats::Digits);
}
