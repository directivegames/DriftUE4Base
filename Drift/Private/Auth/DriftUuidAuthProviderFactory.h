
#pragma once


#include "IDriftAuthProviderFactory.h"


class FDriftUuidAuthProviderFactory : public IDriftAuthProviderFactory
{
public:
    FDriftUuidAuthProviderFactory(int32 instanceIndex, const FString& projectName, const FString& Username, const FString& Password);

    FName GetAuthProviderName() const override;
    TUniquePtr<IDriftAuthProvider> GetAuthProvider() override;

private:
    int32 instanceIndex_;
    FString projectName_;
	FString username_;
	FString password_;
};
