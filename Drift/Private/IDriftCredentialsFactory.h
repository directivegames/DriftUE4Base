// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once


class IDriftCredentialsFactory
{
public:
    virtual void MakeUniqueCredentials(FString& id, FString& password) = 0;
    
    virtual ~IDriftCredentialsFactory() {}
};
