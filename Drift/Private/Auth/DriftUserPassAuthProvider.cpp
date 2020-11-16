
#include "DriftUserPassAuthProvider.h"

#include "ISecureStorage.h"


FDriftUserPassAuthProvider::FDriftUserPassAuthProvider(int32 instanceIndex, TSharedPtr<ISecureStorage> secureStorage, const FString& Username, const FString& Password, bool bAllowAutomaticAccountCreation)
: instanceIndex_(instanceIndex)
, secureStorage_(secureStorage)
, username_(Username)
, password_(Password)
, bAllowAutomaticAccountCreation_(bAllowAutomaticAccountCreation)
{
}


void FDriftUserPassAuthProvider::InitCredentials(TFunction<void(bool)> callback)
{
    callback(true);
}


void FDriftUserPassAuthProvider::GetFriends(TFunction<void(bool, const TArray<TSharedRef<FOnlineFriend>>&)> callback)
{
    // Drift doesn't support friends natively, yet
    callback(true, TArray<TSharedRef<FOnlineFriend>>());
}


void FDriftUserPassAuthProvider::FillProviderDetails(DetailsAppender appender) const
{
    appender(TEXT("username"), username_);
    appender(TEXT("password"), password_);
}


FString FDriftUserPassAuthProvider::ToString() const
{
    return FString::Printf(TEXT("User+Pass: username=%s"), *username_);
}


void FDriftUserPassAuthProvider::GetAvatarUrl(TFunction<void(const FString&)> callback)
{
	callback(TEXT(""));
}
