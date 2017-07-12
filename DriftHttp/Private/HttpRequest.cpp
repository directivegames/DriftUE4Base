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

#include "DriftHttpPCH.h"

#include "Http.h"
#include "HttpRequest.h"
#include "HttpCache.h"
#include "JsonArchive.h"
#include "JsonUtils.h"
#include "ErrorResponse.h"
#include "IErrorReporter.h"


#define LOCTEXT_NAMESPACE "Drift"


HttpRequest::HttpRequest()
: retriesLeft_{ 0 }
, sent_{ FDateTime::UtcNow() }
{
#if !UE_BUILD_SHIPPING
    FPlatformMisc::CreateGuid(guid_);
#endif
}


void HttpRequest::SetCache(TSharedPtr<IHttpCache> cache)
{
    cache_ = cache;
}


void HttpRequest::BindActualRequest(TSharedRef<IHttpRequest> request)
{
    wrappedRequest_ = request;
    wrappedRequest_->OnProcessRequestComplete().BindSP(this, &HttpRequest::InternalRequestCompleted);
}


void HttpRequest::InternalRequestCompleted(FHttpRequestPtr request, FHttpResponsePtr response, bool bWasSuccessful)
{
    if (discarded_)
    {
        OnCompleted.ExecuteIfBound(SharedThis(this));
        return;
    }

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
    if (response.IsValid())
    {
        /**
         * Grab out-of-band debug messages from http response header and display
         * it to the player.
         */
        auto debug_message = response->GetHeader("Drift-Debug-Message");
        if (debug_message.Len())
        {
            OnDebugMessage().ExecuteIfBound(debug_message);
        }
    }
#endif
    
    ResponseContext context(request, response, sent_, false);
    
    if (response.IsValid())
    {
        /**
         * We got a response from the server, let's check if it's good or bad
         */
        if (context.responseCode >= (int32)HttpStatusCodes::Ok && context.responseCode < (int32)HttpStatusCodes::FirstClientError)
        {
            /**
             * We got a non-error response code
             */
            auto contentType = context.response->GetHeader("Content-Type");
            if (!contentType.StartsWith("application/json")) // to handle cases like `application/json; charset=UTF-8`
            {
                context.error = FString::Printf(L"Expected Content-Type 'application/json', but got '%s'", *contentType);
            }
            else
            {
                JsonDocument doc;
                doc.Parse(*response->GetContentAsString());
                if (doc.HasParseError())
                {
                    context.error = FString::Printf(L"JSON response is broken at position %i. RapidJson error: %i",
                        (int32)doc.GetErrorOffset(),
                        (int32)doc.GetParseError());
                }
                else if (expectedResponseCode_ != -1 && context.responseCode != expectedResponseCode_)
                {
                    context.error = FString::Printf(L"Expected '%i', but got '%i'", expectedResponseCode_, context.responseCode);
                    if (doc.HasMember(L"message"))
                    {
                        context.message = doc[L"message"].GetString();
                    }
                }
                else
                {
                    // All default validation passed, process response
                    if (cache_.IsValid() && request->GetVerb() == TEXT("GET"))
                    {
                        cache_->CacheResponse(context);
                    }
                    context.successful = true;
                    OnResponse.ExecuteIfBound(context, doc);
                }
            }
            /**
             * If the error is set, but also handled, that means the caller
             * found some problem and dealt with it itself.
             */
            if (!context.error.IsEmpty() && !context.errorHandled)
            {
                /**
                 * Otherwise, pass it through the error handling chain.
                 */
                BroadcastError(context);
                LogError(context);
            }
            else
            {
                UE_LOG(LogHttpClient, Verbose, TEXT("'%s' SUCCEEDED in %.3f seconds"), *GetAsDebugString(), (FDateTime::UtcNow() - sent_).GetTotalSeconds());
            }
        }
        else
        {
            if (retriesLeft_ > 0 && shouldRetryDelegate_.IsBound() && shouldRetryDelegate_.Execute(request, response))
            {
                Retry();
                return;
            }
            else
            {
                /**
                 * The server returned a non-success response code. Pass it through the error handling chain.
                 */
                BroadcastError(context);
                LogError(context);
            }
        }
    }
    else
    {
        /**
         * The request failed to send, or return. Pass it through the error handling chain.
         */
        BroadcastError(context);
        LogError(context);
    }

    OnCompleted.ExecuteIfBound(SharedThis(this));
}


void HttpRequest::BroadcastError(ResponseContext &context)
{
    DefaultErrorHandler.ExecuteIfBound(context);
    if (!context.errorHandled)
    {
        /**
         * If none of the standard errors applied, give the caller a chance to handle the problem.
         */
        OnError.ExecuteIfBound(context);
        if (!context.errorHandled)
        {
            /**
             * Nobody wants to deal with this, so we do it for them.
             */
            OnUnhandledError.ExecuteIfBound(context);
        }
    }
}


void HttpRequest::LogError(ResponseContext& context)
{
    FString errorMessage;

    auto error = MakeShared<FJsonObject>();
    error->SetNumberField(L"elapsed", (FDateTime::UtcNow() - sent_).GetTotalSeconds());
    error->SetBoolField(L"error_handled", context.errorHandled);
    error->SetNumberField(L"status_code", context.responseCode);
    if (!context.message.IsEmpty())
    {
        error->SetStringField(L"message", context.message);
    }
    if (!context.error.IsEmpty())
    {
        error->SetStringField(L"error", context.error);
    }

    auto requestData = MakeShared<FJsonObject>();
    requestData->SetStringField(L"method", wrappedRequest_->GetVerb());
    requestData->SetStringField(L"url", wrappedRequest_->GetURL());

    auto allHeaders = MakeShared<FJsonObject>();

    auto requestHeaders = context.request->GetAllHeaders();
    if (requestHeaders.Num() > 0)
    {
        for (const auto& header : requestHeaders)
        {
            FString key, value;
            header.Split(L": ", &key, &value);
            allHeaders->SetStringField(key, value);
        }
    }

    if (context.response.IsValid())
    {
        GenericRequestErrorResponse response;
        if (JsonUtils::ParseResponse(context.response, response))
        {
            auto code = response.GetErrorCode();
            if (!code.IsEmpty())
            {
                requestData->SetStringField(L"code", code);
                errorMessage += code;
            }
            auto reason = response.GetErrorReason();
            if (!reason.IsEmpty() && reason != L"undefined")
            {
                requestData->SetStringField(L"reason", reason);
                errorMessage += errorMessage.IsEmpty() ? reason : FString{ L" : " } +reason;
            }
            auto description = response.GetErrorDescription();
            if (!description.IsEmpty())
            {
                requestData->SetStringField(L"description", description);
            }
        }
        requestData->SetStringField(L"data", context.response->GetContentAsString());
        auto responseHeaders = context.response->GetAllHeaders();
        if (responseHeaders.Num() > 0)
        {
            for (const auto& header : responseHeaders)
            {
                FString key, value;
                header.Split(L": ", &key, &value);
                allHeaders->SetStringField(key, value);
            }
        }
    }
    else
    {
        if (context.request->GetStatus() == EHttpRequestStatus::Failed_ConnectionError)
        {
            errorMessage = L"HTTP request timeout";
        }
    }

    if (allHeaders->Values.Num() > 0)
    {
        requestData->SetObjectField("headers", allHeaders);
    }

    error->SetObjectField("request", requestData);
    IErrorReporter::Get()->AddError(L"LogHttpClient",
        errorMessage.IsEmpty() ? L"HTTP request failed" : *errorMessage, error);
}


void HttpRequest::Retry()
{
    UE_LOG(LogHttpClient, Verbose, TEXT("Retrying %s"), *GetAsDebugString());

    --retriesLeft_;
    // Note that we explicitly set the request to be queued for retry
    // the reason is that the internal HTTP processing logic will remove the current request from the system right after this point (Retry is called by the request finish handler)
    // so the only way to make it work is to add the request back in the next tick (this is done automatically by the queue in the request manager)
    Dispatch(true);
}


bool HttpRequest::Dispatch(bool forceQueued)
{
    check(!wrappedRequest_->GetURL().IsEmpty());

    if (cache_.IsValid() && wrappedRequest_->GetVerb() == TEXT("GET"))
    {
        auto header = wrappedRequest_->GetHeader(TEXT("Cache-Control"));
        if (!(header.Contains(TEXT("no-cache")) || header.Contains(TEXT("max-age=0"))))
        {
            auto cachedResponse = cache_->GetCachedResponse(wrappedRequest_->GetURL());
            if (cachedResponse.IsValid())
            {
                ResponseContext context{ wrappedRequest_, cachedResponse, sent_, true };
                JsonDocument doc;
                doc.Parse(*cachedResponse->GetContentAsString());

                OnResponse.ExecuteIfBound(context, doc);
                OnCompleted.ExecuteIfBound(SharedThis(this));

                if (!context.error.IsEmpty() && !context.errorHandled)
                {
                    /**
                     * Otherwise, pass it through the error handling chain.
                     */
                    BroadcastError(context);
                    LogError(context);
                }
                else
                {
                    UE_LOG(LogHttpClient, Verbose, TEXT("'%s' SUCCEEDED from CACHE in %.3f seconds"), *GetAsDebugString(), (FDateTime::UtcNow() - sent_).GetTotalSeconds());
                }

                return true;
            }
        }
    }

    if (OnDispatch.IsBound())
    {
        return OnDispatch.Execute(SharedThis(this), forceQueued);
    }
    return false;
}


void HttpRequest::Discard()
{
    discarded_ = true;

    wrappedRequest_->OnRequestProgress().Unbind();
}


void HttpRequest::Destroy()
{
    Discard();

    wrappedRequest_->CancelRequest();
}


FString HttpRequest::GetAsDebugString() const
{
#if !UE_BUILD_SHIPPING
    return FString::Printf(TEXT("Http Request(%s): %s - %s"), *guid_.ToString(), *wrappedRequest_->GetVerb(), *wrappedRequest_->GetURL());
#else
    return FString::Printf(TEXT("Http Request: %s - %s"), *wrappedRequest_->GetVerb(), *wrappedRequest_->GetURL());
#endif
}


void HttpRequest::SetPayload(const FString& content)
{
    if (!content.IsEmpty())
    {
        wrappedRequest_->SetContentAsString(content);

        if (content.Len())
        {
            SetHeader(TEXT("Content-Type"), contentType_);
        }
    }
}


#undef LOCTEXT_NAMESPACE
