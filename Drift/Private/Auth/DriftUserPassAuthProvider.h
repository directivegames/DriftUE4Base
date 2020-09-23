
#pragma once

#include "IDriftAuthProvider.h"

class ISecureStorage;


class FDriftUserPassAuthProvider : public IDriftAuthProvider
{
public:
    FDriftUserPassAuthProvider(int32 instanceIndex, TSharedPtr<ISecureStorage> secureStorage, const FString& Username, const FString& Password, bool bAllowAutomaticAccountCreation);

    FString GetProviderName() const override { return TEXT("user+pass"); }
    void InitCredentials(InitCredentialsCallback callback) override;
    void GetFriends(GetFriendsCallback callback) override;
    void GetAvatarUrl(GetAvatarUrlCallback callback) override;
    void FillProviderDetails(DetailsAppender appender) const override;

	bool AllowAutomaticAccountCreation() const { return bAllowAutomaticAccountCreation_; }

    FString ToString() const override;

private:
    int32 instanceIndex_;
    TSharedPtr<ISecureStorage> secureStorage_;

    FString username_;
    FString password_;

	bool bAllowAutomaticAccountCreation_;
};
