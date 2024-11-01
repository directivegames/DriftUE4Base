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

#include "ExternalTokenAuthProvider.h"
#include "DriftAPI.h"


FExternalTokenAuthProvider::FExternalTokenAuthProvider()
{
}

FString FExternalTokenAuthProvider::GetProviderName() const
{
    return TokenProviderName;
}

void FExternalTokenAuthProvider::InitCredentials(const FAuthenticationSettings& AuthenticationSettings, InitCredentialsCallback callback)
{
    if (AuthenticationSettings.ExternalTokenData &&
        AuthenticationSettings.ExternalTokenData->Token.Len() &&
        AuthenticationSettings.ExternalTokenData->TokenProviderName.Len())
    {
        bAllowAutomaticAccountCreation = AuthenticationSettings.bAutoCreateAccount;
        TokenProviderName = AuthenticationSettings.ExternalTokenData->TokenProviderName;
        Token = AuthenticationSettings.ExternalTokenData->Token;
        callback(true);
    }
    else
    {
        callback(false);
    }    
}

void FExternalTokenAuthProvider::GetFriends(GetFriendsCallback callback)
{
    callback(true, {});
}

void FExternalTokenAuthProvider::GetAvatarUrl(GetAvatarUrlCallback callback)
{
    callback(TEXT(""));
}

void FExternalTokenAuthProvider::FillProviderDetails(DetailsAppender appender) const
{
    appender(TEXT("token"), Token);
}

FString FExternalTokenAuthProvider::ToString() const
{
    return TEXT("");
}
