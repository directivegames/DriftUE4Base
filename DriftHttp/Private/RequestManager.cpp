/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2016-2017 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/

#include "RequestManager.h"
#include "HttpRequest.h"
#include "HttpCache.h"
#include "JsonArchive.h"


DEFINE_LOG_CATEGORY(LogHttpClient);


#if PLATFORM_APPLE
#include "CFNetwork/CFNetworkErrors.h"
#endif


RequestManager::RequestManager()
    : defaultRetries_{ 0 }
    , maxConcurrentRequests_{ MAX_int32 }
{
}


RequestManager::~RequestManager()
{
    for (auto& request : activeRequests_)
    {
        request->Discard();
    }
}


TSharedRef<HttpRequest> RequestManager::Get(const FString& url)
{
    return Get(url, HttpStatusCodes::Ok);
}


TSharedRef<HttpRequest> RequestManager::Get(const FString& url, HttpStatusCodes expectedResponseCode)
{
    return CreateRequest(HttpMethods::XGET, url, expectedResponseCode);
}


TSharedRef<HttpRequest> RequestManager::Delete(const FString& url)
{
    return Delete(url, HttpStatusCodes::Ok);
}


TSharedRef<HttpRequest> RequestManager::Delete(const FString& url, HttpStatusCodes expectedResponseCode)
{
    return CreateRequest(HttpMethods::XDELETE, url, expectedResponseCode);
}


TSharedRef<HttpRequest> RequestManager::Patch(const FString& url, const FString& payload)
{
    return Patch(url, payload, HttpStatusCodes::Ok);
}


TSharedRef<HttpRequest> RequestManager::Patch(const FString& url, const FString& payload, HttpStatusCodes expectedResponseCode)
{
    return CreateRequest(HttpMethods::XPATCH, url, payload, expectedResponseCode);
}


TSharedRef<HttpRequest> RequestManager::Post(const FString& url, const FString& payload)
{
    return Post(url, payload, HttpStatusCodes::Created);
}


TSharedRef<HttpRequest> RequestManager::Post(const FString& url, const FString& payload, HttpStatusCodes expectedResponseCode)
{
    return CreateRequest(HttpMethods::XPOST, url, payload, expectedResponseCode);
}


TSharedRef<HttpRequest> RequestManager::Put(const FString& url, const FString& payload)
{
    return Put(url, payload, HttpStatusCodes::Ok);
}


TSharedRef<HttpRequest> RequestManager::Put(const FString& url, const FString& payload, HttpStatusCodes expectedResponseCode)
{
    return CreateRequest(HttpMethods::XPUT, url, payload, expectedResponseCode);
}


TSharedRef<HttpRequest> RequestManager::CreateRequest(HttpMethods method, const FString& url)
{
    return CreateRequest(method, url, HttpStatusCodes::Undefined);
}


TSharedRef<HttpRequest> RequestManager::CreateRequest(HttpMethods method, const FString& url, HttpStatusCodes expectedResponseCode)
{
    static const FString verbs[] =
    {
        TEXT("GET"),
        TEXT("PUT"),
        TEXT("POST"),
        TEXT("PATCH"),
        TEXT("DELETE"),
        TEXT("HEAD"),
        TEXT("OPTIONS"),
    };

    check(IsInGameThread());

    TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();
    request->SetURL(url);
    request->SetVerb(verbs[(int)method]);

    TSharedRef<HttpRequest> wrapper(new HttpRequest());
    wrapper->BindActualRequest(request);
    wrapper->expectedResponseCode_ = (int32)expectedResponseCode;

    wrapper->DefaultErrorHandler = DefaultErrorHandler;
    wrapper->OnUnhandledError = DefaultUnhandledErrorHandler;
    wrapper->OnDriftDeprecationMessage = DefaultDriftDeprecationMessageHandler;

    wrapper->SetCache(cache_);

    AddCustomHeaders(wrapper);

    if (userContext_.Num() > 0)
    {
        JsonValue temp(rapidjson::kObjectType);
        for (const auto& item : userContext_)
        {
            JsonArchive::AddMember(temp, *item.Key, *item.Value);
        }
        
#if !UE_BUILD_SHIPPING
        JsonArchive::AddMember(temp, TEXT("request_id"), *wrapper->RequestID().ToString());
#endif
        FString contextValue;
        JsonArchive::SaveObject(temp, contextValue);
        wrapper->SetHeader(TEXT("Drift-Log-Context"), contextValue);
    }
    
    wrapper->SetRetries(defaultRetries_);
    wrapper->OnShouldRetry().BindRaw(this, &RequestManager::ShouldRetryCallback);

    wrapper->OnDispatch.BindSP(this, &RequestManager::ProcessRequest);
    wrapper->OnCompleted.BindSP(this, &RequestManager::OnRequestFinished);

    UE_LOG(LogHttpClient, Verbose, TEXT("'%s' CREATED"), *wrapper->GetAsDebugString());

    return wrapper;
}


TSharedRef<HttpRequest> RequestManager::CreateRequest(HttpMethods method, const FString& url, const FString& payload)
{
    return CreateRequest(method, url, payload, HttpStatusCodes::Undefined);
}


TSharedRef<HttpRequest> RequestManager::CreateRequest(HttpMethods method, const FString& url, const FString& payload, HttpStatusCodes expectedResponseCode)
{
    auto request = CreateRequest(method, url, expectedResponseCode);
    request->SetPayload(payload);
    return request;
}


void RequestManager::OnRequestFinished(TSharedRef<HttpRequest> request)
{
    check(IsInGameThread());

    activeRequests_.RemoveSingleSwap(request);
}


bool RequestManager::ShouldRetryCallback(FHttpRequestPtr request, FHttpResponsePtr response) const
{
    // TODO: fix the condition
    int32 errorCode = -1;//TODO: response->GetErrorCode();
#if PLATFORM_APPLE
    if (errorCode == kCFURLErrorTimedOut)
    {
        return true;
    }
#elif PLATFORM_LINUX
    //TODO: figure out which error code to use
    return false;
#elif PLATFORM_WINDOWS
    //TODO: figure out which error code to use
    return false;
#elif PLATFORM_PS4
	//TODO: figure out which error code to use
	return false;
#elif PLATFORM_ANDROID
	//TODO: figure out which error code to use
	return false;
#else
#error "Error code not checked for the current platform"
#endif

    return false;
}


bool RequestManager::ProcessRequest(TSharedRef<HttpRequest> request, bool forceQueued)
{
    check(IsInGameThread());

    if (activeRequests_.Num() < maxConcurrentRequests_ && !forceQueued)
    {
        UE_LOG(LogHttpClient, Verbose, TEXT("'%s' DISPATCHED"), *request->GetAsDebugString());

        activeRequests_.Add(request);
        return request->wrappedRequest_->ProcessRequest();
    }
    else
    {
        UE_LOG(LogHttpClient, Verbose, TEXT("'%s' QUEUED"), *request->GetAsDebugString());

        queuedRequests_.Enqueue(request);
        return true;
    }
}


void RequestManager::SetCache(TSharedPtr<IHttpCache> cache)
{
    cache_ = cache;
}


void RequestManager::SetLogContext(TMap<FString, FString>&& context)
{
    userContext_ = Forward<TMap<FString, FString>>(context);
}


void RequestManager::UpdateLogContext(TMap<FString, FString>& context)
{
    userContext_.Append(context);
}


void RequestManager::Tick(float DeltaTime)
{
    TSharedPtr<HttpRequest> request;
    while ((activeRequests_.Num() < maxConcurrentRequests_) && queuedRequests_.Dequeue(request))
    {
        activeRequests_.Add(request.ToSharedRef());
        request->wrappedRequest_->ProcessRequest();

        UE_LOG(LogHttpClient, Verbose, TEXT("'%s' DISPATCHED"), *request->GetAsDebugString());
    }
}


TStatId RequestManager::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(RequestManager, STATGROUP_Tickables);
}
