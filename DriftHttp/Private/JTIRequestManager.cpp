// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#include "DriftHttpPCH.h"

#include "JTIRequestManager.h"


JTIRequestManager::JTIRequestManager(const FString& jti)
: headerValue(TEXT("JTI ") + jti)
{
}


void JTIRequestManager::AddCustomHeaders(TSharedRef<HttpRequest> request) const
{
    JsonRequestManager::AddCustomHeaders(request);

    request->SetHeader(TEXT("Authorization"), headerValue);
}
