// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

#pragma once


class ISecureStorage
{
public:
	virtual bool SaveValue(const FString& key, const FString& value, bool overwrite) = 0;
	virtual bool GetValue(const FString& key, FString& value) = 0;

    virtual ~ISecureStorage() {}
};
