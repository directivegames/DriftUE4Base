
#pragma once


#include "IDriftAuthProviderFactory.h"


class FDriftUserPassAuthProviderFactory : public IDriftAuthProviderFactory
{
public:
    FDriftUserPassAuthProviderFactory(int32 instanceIndex, const FString& projectName, const FString& Username, const FString& Password, bool bAllowAutomaticAccountCreation);

    FName GetAuthProviderName() const override;
    TUniquePtr<IDriftAuthProvider> GetAuthProvider() override;

private:
    int32 instanceIndex_;
    FString projectName_;
	FString username_;
	FString password_;
	bool bAllowAutomaticAccountCreation_;
};
