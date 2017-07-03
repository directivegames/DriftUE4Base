// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "IDriftCredentialsFactory.h"


class FDriftCredentialsFactory : public IDriftCredentialsFactory
{
public:
    void MakeUniqueCredentials(FString& id, FString& password) override;
};
