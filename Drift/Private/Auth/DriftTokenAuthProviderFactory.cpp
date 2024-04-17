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

#include "DriftTokenAuthProviderFactory.h"

#include "CommandLineArgumentAccessTokenSource.h"
#include "DriftTokenAuthProvider.h"
#include "IDriftAccessTokenSource.h"
#include "Features/IModularFeatures.h"

FDriftTokenAuthProviderFactory::FDriftTokenAuthProviderFactory()
{
    IModularFeatures::Get().RegisterModularFeature(AccessTokenSourceName, &AccessTokenArgSource);
    IModularFeatures::Get().RegisterModularFeature(AccessTokenSourceName, &JwtArgSource);
    IModularFeatures::Get().RegisterModularFeature(AccessTokenSourceName, &JtiArgSource);
}

FDriftTokenAuthProviderFactory::~FDriftTokenAuthProviderFactory()
{
    IModularFeatures::Get().UnregisterModularFeature(AccessTokenSourceName, &AccessTokenArgSource);
    IModularFeatures::Get().UnregisterModularFeature(AccessTokenSourceName, &JwtArgSource);
    IModularFeatures::Get().UnregisterModularFeature(AccessTokenSourceName, &JtiArgSource);
}

FName FDriftTokenAuthProviderFactory::GetAuthProviderName() const
{
    return TEXT("jwt");
}

TUniquePtr<IDriftAuthProvider> FDriftTokenAuthProviderFactory::GetAuthProvider()
{
    IModularFeatures::Get().IsModularFeatureAvailable(AccessTokenSourceName);
    auto TokenSources = IModularFeatures::Get().GetModularFeatureImplementations<
        IDriftAccessTokenSource>(AccessTokenSourceName);
    return MakeUnique<FDriftTokenAuthProvider>(TokenSources);
}
