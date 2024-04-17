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
#include "IDriftAccessTokenSource.h"
#include "IDriftAuthProvider.h"

class FDriftTokenAuthProvider : public IDriftAuthProvider
{
public:
    FDriftTokenAuthProvider(const TArray<IDriftAccessTokenSource*>& TokenSources);

    FString GetProviderName() const override;
    void InitCredentials(InitCredentialsCallback callback) override;
    void GetFriends(GetFriendsCallback callback) override;
    void GetAvatarUrl(GetAvatarUrlCallback callback) override;
    void FillProviderDetails(DetailsAppender appender) const override;
    FString ToString() const override;

private:
    FString token_;
    TArray<IDriftAccessTokenSource*> tokenSources_;
};

