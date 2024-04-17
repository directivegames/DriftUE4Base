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

#include "DriftTokenAuthProvider.h"

FDriftTokenAuthProvider::FDriftTokenAuthProvider(const TArray<IDriftAccessTokenSource*>& TokenSources)
    : tokenSources_{ TokenSources }
{
}

FString FDriftTokenAuthProvider::GetProviderName() const
{
    return TEXT("jwt");
}

void FDriftTokenAuthProvider::InitCredentials(InitCredentialsCallback callback)
{
    for (const auto TokenSource : tokenSources_)
    {
        if (token_.IsEmpty())
        {
            token_ = TokenSource->GetToken();
        }
    }

    callback(!token_.IsEmpty());
}

void FDriftTokenAuthProvider::GetFriends(GetFriendsCallback callback)
{
    callback(true, {});
}

void FDriftTokenAuthProvider::GetAvatarUrl(GetAvatarUrlCallback callback)
{
    callback(TEXT(""));
}

void FDriftTokenAuthProvider::FillProviderDetails(DetailsAppender appender) const
{
    appender(TEXT("jwt"), token_);
}

FString FDriftTokenAuthProvider::ToString() const
{
    return TEXT("");
}
