// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#pragma once


class ISecureStorage;


class SecureStorageFactory
{
public:
	static TSharedPtr<ISecureStorage> GetSecureStorage(const FString& productName, const FString& serviceName);
};
