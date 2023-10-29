/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2016-2021 Directive Games Limited. All Rights Reserved.
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
#include "HttpModule.h"
#include "JsonArchive.h"


DEFINE_LOG_CATEGORY(LogHttpClient);

#ifndef PLATFORM_PS4
#define PLATFORM_PS4 0
#endif


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
	for (const auto& Request : activeRequests_)
	{
		Request->Discard();
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


TSharedRef<HttpRequest> RequestManager::Patch(const FString& url, const FString& payload
                                              , HttpStatusCodes expectedResponseCode)
{
	return CreateRequest(HttpMethods::XPATCH, url, payload, expectedResponseCode);
}


TSharedRef<HttpRequest> RequestManager::Post(const FString& url, const FString& payload)
{
	return Post(url, payload, HttpStatusCodes::Created);
}


TSharedRef<HttpRequest> RequestManager::Post(const FString& url, const FString& payload
                                             , HttpStatusCodes expectedResponseCode)
{
	return CreateRequest(HttpMethods::XPOST, url, payload, expectedResponseCode);
}


TSharedRef<HttpRequest> RequestManager::Put(const FString& url, const FString& payload)
{
	return Put(url, payload, HttpStatusCodes::Ok);
}


TSharedRef<HttpRequest> RequestManager::Put(const FString& url, const FString& payload
                                            , HttpStatusCodes expectedResponseCode)
{
	return CreateRequest(HttpMethods::XPUT, url, payload, expectedResponseCode);
}


TSharedRef<HttpRequest> RequestManager::CreateRequest(HttpMethods method, const FString& url)
{
	return CreateRequest(method, url, HttpStatusCodes::Undefined);
}


TSharedRef<HttpRequest> RequestManager::CreateRequest(HttpMethods method, const FString& url
                                                      , HttpStatusCodes expectedResponseCode)
{
	static const FString Verbs[] =
	{
		TEXT("GET"), TEXT("PUT"), TEXT("POST"), TEXT("PATCH"), TEXT("DELETE"), TEXT("HEAD"), TEXT("OPTIONS"),
	};

	check(IsInGameThread());

	const auto Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(url);
	Request->SetVerb(Verbs[static_cast<int>(method)]);

	TSharedRef<HttpRequest> Wrapper(new HttpRequest());
	Wrapper->BindActualRequest(Request);
	Wrapper->expectedResponseCode_ = static_cast<int32>(expectedResponseCode);

	Wrapper->DefaultErrorHandler = DefaultErrorHandler;
	Wrapper->OnUnhandledError = DefaultUnhandledErrorHandler;
	Wrapper->OnDriftDeprecationMessage = DefaultDriftDeprecationMessageHandler;

	Wrapper->SetCache(cache_);

	AddCustomHeaders(Wrapper);

	if (userContext_.Num() > 0)
	{
		JsonValue Temp(rapidjson::kObjectType);
		for (const auto& item : userContext_)
		{
			Temp.SetField(item.Key, item.Value);
		}

#if !UE_BUILD_SHIPPING
		Temp.SetField(TEXT("request_id"), *Wrapper->RequestID().ToString());
#endif
		FString ContextValue;
		JsonArchive::SaveObject(Temp, ContextValue);
		Wrapper->SetHeader(TEXT("Drift-Log-Context"), ContextValue);
	}

	Wrapper->SetRetries(defaultRetries_);
	Wrapper->OnShouldRetry().BindRaw(this, &RequestManager::ShouldRetryCallback);

	Wrapper->OnDispatch.BindSP(this, &RequestManager::ProcessRequest);
	Wrapper->OnRetry.BindSP(this, &RequestManager::EnqueueRequest);
	Wrapper->OnCompleted.BindSP(this, &RequestManager::OnRequestFinished);

	UE_LOG(LogHttpClient, Verbose, TEXT("'%s' CREATED"), *Wrapper->GetAsDebugString());

	return Wrapper;
}


TSharedRef<HttpRequest> RequestManager::CreateRequest(HttpMethods method, const FString& url, const FString& payload)
{
	return CreateRequest(method, url, payload, HttpStatusCodes::Undefined);
}


TSharedRef<HttpRequest> RequestManager::CreateRequest(HttpMethods method, const FString& url, const FString& payload
                                                      , HttpStatusCodes expectedResponseCode)
{
	auto Request = CreateRequest(method, url, expectedResponseCode);
	Request->SetPayload(payload);
	return Request;
}


TSharedRef<HttpRequest> RequestManager::CreateRequest(HttpMethods method, const FString& url, const TArray<uint8>& payload)
{
    return CreateRequest(method, url, payload, HttpStatusCodes::Undefined);
}


TSharedRef<HttpRequest> RequestManager::CreateRequest(HttpMethods method, const FString& url, const TArray<uint8>& payload
    , HttpStatusCodes expectedResponseCode)
{
    auto Request = CreateRequest(method, url, expectedResponseCode);
    Request->SetContent(payload);
    return Request;
}


void RequestManager::OnRequestFinished(TSharedRef<HttpRequest> request)
{
	check(IsInGameThread());

	activeRequests_.RemoveSingleSwap(request);
}


bool RequestManager::ShouldRetryCallback(FHttpRequestPtr request, FHttpResponsePtr response) const
{
	// TODO: fix the condition
	int32 ErrorCode = -1; //TODO: response->GetErrorCode();
#if PLATFORM_APPLE
    if (ErrorCode == kCFURLErrorTimedOut)
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
#elif PLATFORM_HOLOLENS
	//TODO: figure out which error code to use
	return false;
#elif PLATFORM_SWITCH
//TODO: figure out which error code to use
	return false;
#else
#error "Error code not checked for the current platform"
#endif

	return false;
}


bool RequestManager::ProcessRequest(TSharedRef<HttpRequest> request)
{
	check(IsInGameThread());

	if (activeRequests_.Num() >= maxConcurrentRequests_)
	{
		UE_LOG(LogHttpClient, Verbose, TEXT("'%s' QUEUED"), *request->GetAsDebugString());

		queuedRequests_.Enqueue(request);
		return true;
	}

	UE_LOG(LogHttpClient, Verbose, TEXT("'%s' DISPATCHED"), *request->GetAsDebugString());

	activeRequests_.Add(request);
	return request->wrappedRequest_->ProcessRequest();
}


bool RequestManager::EnqueueRequest(TSharedRef<HttpRequest> Request, float Delay)
{
	check(IsInGameThread());

	pendingRetries_.Emplace(TPair<FDateTime, TSharedPtr<HttpRequest>>{
		FDateTime::Now() + FTimespan::FromSeconds(Delay), Request
	});
	return true;
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
	// Check pending retries before processing new requests
	while (activeRequests_.Num() < maxConcurrentRequests_)
	{
		const auto OverdueRequest = pendingRetries_.FindByPredicate([](const auto& Entry)
		{
			return Entry.Key < FDateTime::Now();
		});

		if (!OverdueRequest)
		{
			break;
		}

		const auto& Request = activeRequests_.Add_GetRef(OverdueRequest->Value.ToSharedRef());
		pendingRetries_.RemoveSingleSwap(*OverdueRequest);
		Request->wrappedRequest_->ProcessRequest();
	}

	// Check any queued requests
	TSharedPtr<HttpRequest> Request;
	while (activeRequests_.Num() < maxConcurrentRequests_ && queuedRequests_.Dequeue(Request))
	{
		activeRequests_.Add(Request.ToSharedRef());
		Request->wrappedRequest_->ProcessRequest();

		UE_LOG(LogHttpClient, Verbose, TEXT("'%s' DISPATCHED"), *Request->GetAsDebugString());
	}
}


TStatId RequestManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(RequestManager, STATGROUP_Tickables);
}
