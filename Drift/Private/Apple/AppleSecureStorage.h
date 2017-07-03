// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once


#include "ISecureStorage.h"


#if PLATFORM_APPLE


class AppleSecureStorage : public ISecureStorage
{
public:
	AppleSecureStorage(const FString& productName, const FString& serviceName);

	bool SaveValue(const FString& key, const FString& value, bool overwrite) override;
	bool GetValue(const FString& key, FString& value) override;

private:
	FString MakeProductKey(const FString& key);

	FString productName_;
	FString serviceName_;
};


#endif // PLATFORM_APPLE
