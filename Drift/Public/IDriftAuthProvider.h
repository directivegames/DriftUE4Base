
#pragma once

class FOnlineFriend;


class IDriftAuthProvider : public TSharedFromThis<IDriftAuthProvider>
{
public:
    using InitCredentialsCallback = TFunction<void(bool)>;
    using GetFriendsCallback = TFunction<void(bool, const TArray<TSharedRef<FOnlineFriend>>&)>;
    using GetAvatarUrlCallback = TFunction<void(const FString&)>;

    using DetailsAppender = TFunction<void(const FString&, const FString&)>;

public:
    virtual FString GetProviderName() const = 0;
    virtual void InitCredentials(InitCredentialsCallback callback) = 0;
    virtual void GetFriends(GetFriendsCallback callback) = 0;
	virtual void GetAvatarUrl(GetAvatarUrlCallback callback) = 0;
    virtual void FillProviderDetails(DetailsAppender appender) const = 0;
    virtual FString GetNickname() { return L""; }

    virtual FString ToString() const = 0;

    virtual ~IDriftAuthProvider() {}
};
