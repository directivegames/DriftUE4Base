// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#include "AppleSecureStorage.h"


#if PLATFORM_APPLE


#include "Apple/SSKeychain/SSKeychain.h"


AppleSecureStorage::AppleSecureStorage(const FString& productName, const FString& serviceName)
: productName_{ productName }
, serviceName_{ serviceName }
{
}


bool AppleSecureStorage::SaveValue(const FString& key, const FString& value, bool overwrite)
{
    FString temp;
    
    if (overwrite || !GetValue(key, temp))
    {
        NSString* nsServiceName = [NSString stringWithFString:serviceName_];
        NSString* nsKey = [NSString stringWithFString:MakeProductKey(key)];
        NSString* nsValue = [NSString stringWithFString:value];
        [SSKeychain setPassword:nsValue forService:nsServiceName account:nsKey];
        return true;
    }
    return false;
}


bool AppleSecureStorage::GetValue(const FString& key, FString& value)
{
    NSString* nsServiceName = [NSString stringWithFString:serviceName_];
    NSString* nsKey = [NSString stringWithFString:MakeProductKey(key)];
    NSString* nsValue = [SSKeychain passwordForService:nsServiceName account:nsKey];
    
    if (nsValue != nil)
    {
        value = nsValue;
        return true;
    }
    return false;
}


FString AppleSecureStorage::MakeProductKey(const FString& key)
{
    return FString::Printf(TEXT("%s::%s"), *productName_, *key);
}


#endif // PLATFORM_APPLE
