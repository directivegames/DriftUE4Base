// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

#include "DriftHttpPCH.h"

#include "JWTRequestManager.h"


JWTRequestManager::JWTRequestManager(const FString& token)
: headerValue(TEXT("Bearer ") + token)
{
}


void JWTRequestManager::AddCustomHeaders(TSharedRef<HttpRequest> request) const
{
    JsonRequestManager::AddCustomHeaders(request);
    
    request->SetHeader(TEXT("Authorization"), headerValue);
}
