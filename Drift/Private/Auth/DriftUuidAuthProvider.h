
#pragma once

#include "IDriftAuthProvider.h"

class IDriftCredentialsFactory;
class ISecureStorage;


class FDriftUuidAuthProvider : public IDriftAuthProvider
{
public:
    FDriftUuidAuthProvider(int32 instanceIndex, TUniquePtr<IDriftCredentialsFactory> credentialsFactory, TSharedPtr<ISecureStorage> secureStorage);
    
    FString GetProviderName() const override { return TEXT("uuid"); }
    void InitCredentials(InitCredentialsCallback callback) override;
    void GetFriends(GetFriendsCallback callback) override;
    void GetAvatarUrl(GetAvatarUrlCallback callback) override;
    void FillProviderDetails(DetailsAppender appender) const override;

    FString ToString() const override;

private:
    void GetDeviceIDCredentials();

private:
    int32 instanceIndex_;
    TUniquePtr<IDriftCredentialsFactory> credentialsFactory_;
    TSharedPtr<ISecureStorage> secureStorage_;

    FString key_;
    FString secret_;
};
