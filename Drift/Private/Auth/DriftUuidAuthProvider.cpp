
#include "DriftPrivatePCH.h"

#include "DriftUuidAuthProvider.h"
#include "IDriftCredentialsFactory.h"
#include "ISecureStorage.h"


FDriftUuidAuthProvider::FDriftUuidAuthProvider(int32 instanceIndex, TUniquePtr<IDriftCredentialsFactory> credentialsFactory, TSharedPtr<ISecureStorage> secureStorage)
: instanceIndex_(instanceIndex)
, credentialsFactory_(credentialsFactory.Release())
, secureStorage_(secureStorage)
{
}


void FDriftUuidAuthProvider::InitCredentials(TFunction<void(bool)> callback)
{
    GetDeviceIDCredentials();
    callback(true);
}


void FDriftUuidAuthProvider::GetFriends(TFunction<void(bool, const TArray<TSharedRef<FOnlineFriend>>&)> callback)
{
    // Drift doesn't support friends natively, yet
    callback(true, TArray<TSharedRef<FOnlineFriend>>());
}


void FDriftUuidAuthProvider::FillProviderDetails(DetailsAppender appender) const
{
    appender(TEXT("key"), key_);
    appender(TEXT("secret"), secret_);
}


FString FDriftUuidAuthProvider::ToString() const
{
    return FString::Printf(TEXT("Device ID: key=%s"), *key_);
}


void FDriftUuidAuthProvider::GetDeviceIDCredentials()
{
    auto instanceIndex = instanceIndex_;
    if (instanceIndex == 0)
    {
        FString instanceIndexString;
        if (FParse::Value(FCommandLine::Get(), TEXT("-uuid_index="), instanceIndexString))
        {
            instanceIndex = FCString::Atoi(*instanceIndexString);
        }
    }
    FString keyIndex = instanceIndex == 0 ? TEXT("") : FString::Printf(TEXT("_%d"), instanceIndex);
    FString deviceIDKey = FString::Printf(TEXT("device_id%s"), *keyIndex);
    FString devicePasswordKey = FString::Printf(TEXT("device_password%s"), *keyIndex);
    FString deviceID;
    FString devicePassword;

    if (!secureStorage_->GetValue(deviceIDKey, deviceID) || deviceID.StartsWith(TEXT("device:")))
    {
        credentialsFactory_->MakeUniqueCredentials(deviceID, devicePassword);
        secureStorage_->SaveValue(deviceIDKey, deviceID, true);
        secureStorage_->SaveValue(devicePasswordKey, devicePassword, true);
    }
    else
    {
        secureStorage_->GetValue(devicePasswordKey, devicePassword);
    }
    key_ = deviceID;
    secret_ = devicePassword;
}

void FDriftUuidAuthProvider::GetAvatarUrl(TFunction<void(const FString&)> callback)
{
	callback(TEXT(""));
}
