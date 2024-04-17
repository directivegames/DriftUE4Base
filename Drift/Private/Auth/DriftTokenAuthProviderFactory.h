/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2024 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/

#pragma once
#include "CommandLineArgumentAccessTokenSource.h"
#include "IDriftAuthProviderFactory.h"

const FName AccessTokenSourceName = TEXT("AccessTokenSource");

class FDriftTokenAuthProviderFactory : public IDriftAuthProviderFactory
{
public:
    FDriftTokenAuthProviderFactory();
    ~FDriftTokenAuthProviderFactory() override;

    FName GetAuthProviderName() const override;
    TUniquePtr<IDriftAuthProvider> GetAuthProvider() override;

private:
    FCommandLineArgumentAccessTokenSource AccessTokenArgSource{TEXT("access_token")};
    FCommandLineArgumentAccessTokenSource JwtArgSource{TEXT("jwt")};
    FCommandLineArgumentAccessTokenSource JtiArgSource{TEXT("jti")};
};
