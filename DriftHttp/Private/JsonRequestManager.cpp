/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2016-2019 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/


#include "JsonRequestManager.h"


void JsonRequestManager::AddCustomHeaders(TSharedRef<HttpRequest> request) const
{
    RequestManager::AddCustomHeaders(request);
    request->SetHeader(TEXT("Accept"), TEXT("application/json"));
    if (request->GetRequestURL().Find(TEXT("http://localhost:")) != 0) // don't set API Key if we're using localhost as backend
    {
        request->SetHeader(TEXT("Drift-Api-Key"), apiKey_);
    }
    // don't set the header directly because we don't know if the request is going to carry any content
    request->SetContentType(TEXT("application/json"));
}


template<>
TSharedRef<HttpRequest> JsonRequestManager::CreateRequest<FString>(HttpMethods method, const FString& url, const FString& payload)
{
    return CreateRequest(method, url, payload, HttpStatusCodes::Undefined);
}


template<>
TSharedRef<HttpRequest> JsonRequestManager::CreateRequest<FString>(HttpMethods method, const FString& url, const FString& payload, HttpStatusCodes expectedCode)
{
    return RequestManager::CreateRequest(method, url, payload, expectedCode);
}


void JsonRequestManager::SetApiKey(const FString& apiKey)
{
    apiKey_ = apiKey;
}
