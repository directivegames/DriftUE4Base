
#include "DriftUserPassAuthProviderFactory.h"

#include "DriftUserPassAuthProvider.h"
#include "SecureStorageFactory.h"


FDriftUserPassAuthProviderFactory::FDriftUserPassAuthProviderFactory(int32 instanceIndex, const FString& projectName, const FString& Username, const FString& Password, bool bAllowAutomaticAccountCreation)
: instanceIndex_(instanceIndex)
, projectName_(projectName)
, username_(Username)
, password_(Password)
, bAllowAutomaticAccountCreation_(bAllowAutomaticAccountCreation)
{

}


FName FDriftUserPassAuthProviderFactory::GetAuthProviderName() const
{
	return FName(TEXT("user+pass"));
}


TUniquePtr<IDriftAuthProvider> FDriftUserPassAuthProviderFactory::GetAuthProvider()
{
    return MakeUnique<FDriftUserPassAuthProvider>(instanceIndex_, SecureStorageFactory::GetSecureStorage(projectName_, TEXT("Drift")), username_, password_, bAllowAutomaticAccountCreation_);
}
