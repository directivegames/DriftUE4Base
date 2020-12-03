
#pragma once

#include "IDriftAuthProvider.h"

class IDriftCredentialsFactory;
class ISecureStorage;


class FDriftUuidAuthProvider : public IDriftAuthProvider
{
public:
    FDriftUuidAuthProvider(int32 instanceIndex, TUniquePtr<IDriftCredentialsFactory> credentialsFactory, TSharedPtr<ISecureStorage> secureStorage);

    virtual FString GetProviderName() const override { return TEXT("uuid"); }
    virtual void InitCredentials(InitCredentialsCallback callback) override;
    virtual void GetFriends(GetFriendsCallback callback) override;
    virtual void GetAvatarUrl(GetAvatarUrlCallback callback) override;
    virtual void FillProviderDetails(DetailsAppender appender) const override;

    virtual FString ToString() const override;

private:
    void GetDeviceIDCredentials();

private:
    int32 instanceIndex_;
    TUniquePtr<IDriftCredentialsFactory> credentialsFactory_;
    TSharedPtr<ISecureStorage> secureStorage_;

    FString key_;
    FString secret_;
};
