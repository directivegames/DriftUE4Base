
#pragma once


#include "IDriftAuthProviderFactory.h"


class FDriftUuidAuthProviderFactory : public IDriftAuthProviderFactory
{
public:
    FDriftUuidAuthProviderFactory(int32 instanceIndex, const FString& projectName);

    FName GetAuthProviderName() const override;
    TUniquePtr<IDriftAuthProvider> GetAuthProvider() override;

private:
    int32 instanceIndex_;
    FString projectName_;
};
