
#include "DriftUuidAuthProviderFactory.h"

#include "DriftUuidAuthProvider.h"
#include "DriftCredentialsFactory.h"
#include "SecureStorageFactory.h"


FDriftUuidAuthProviderFactory::FDriftUuidAuthProviderFactory(int32 instanceIndex, const FString& projectName, const FString& Username, const FString& Password)
: instanceIndex_(instanceIndex)
, projectName_(projectName)
, username_(Username)
, password_(Password)
{

}


FName FDriftUuidAuthProviderFactory::GetAuthProviderName() const
{
	return FName(TEXT("uuid"));
}


TUniquePtr<IDriftAuthProvider> FDriftUuidAuthProviderFactory::GetAuthProvider()
{
    return MakeUnique<FDriftUuidAuthProvider>(instanceIndex_, MakeUnique<FDriftCredentialsFactory>(), SecureStorageFactory::GetSecureStorage(projectName_, TEXT("Drift")), username_, password_);
}
