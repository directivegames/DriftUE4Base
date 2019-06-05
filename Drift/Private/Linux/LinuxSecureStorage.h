// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#pragma once


#include "ISecureStorage.h"


#if PLATFORM_LINUX


class LinuxSecureStorage : public ISecureStorage
{
public:
	LinuxSecureStorage(const FString& productName, const FString& serviceName);

	bool SaveValue(const FString& key, const FString& value, bool overwrite) override;
	bool GetValue(const FString& key, FString& value) override;

private:
	FString productName_;
	FString serviceName_;
};


#endif // PLATFORM_LINUX
