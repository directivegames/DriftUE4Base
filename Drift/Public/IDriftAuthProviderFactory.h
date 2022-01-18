
#pragma once

#include "Features/IModularFeatures.h"

class IDriftAuthProvider;


class IDriftAuthProviderFactory : public IModularFeature
{
public:
	virtual ~IDriftAuthProviderFactory() {}

    virtual FName GetAuthProviderName() const = 0;
    virtual TUniquePtr<IDriftAuthProvider> GetAuthProvider() = 0;

#if WITH_EDITOR
	virtual bool IsSupportedInPIE() const { return false; }
#endif // WITH_EDITOR
};
