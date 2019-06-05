// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#include "DriftHttpPCH.h"

#include "JsonRequestManager.h"


void JsonRequestManager::AddCustomHeaders(TSharedRef<HttpRequest> request) const
{
    RequestManager::AddCustomHeaders(request);
    request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	request->SetHeader(TEXT("Drift-Api-Key"), apiKey_);
	if (tenantOverride_.Len())
	{
		request->SetHeader(TEXT("Drift-Tenant"), tenantOverride_);
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


void JsonRequestManager::SetTenantOverride(const FString& tenantOverride)
{
	tenantOverride_ = tenantOverride;
}
