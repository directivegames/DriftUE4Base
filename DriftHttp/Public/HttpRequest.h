/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2016-2018 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/

#pragma once

#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonArchive.h"


class IHttpCache;


// based on http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
enum class HttpStatusCodes
{
    Ok = 200,
    Created = 201,
    Accepted = 202,
    NoContent = 204,
    Moved = 301,
    Found = 302,
    SeeOther = 303,
    NotModified = 304,
    BadRequest = 400,
    Unauthorized = 401,
    Forbidden = 403,
    NotFound = 404,
    NotAllowed = 405,
    NotAcceptable = 406,
    Timeout = 408,
    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,

    FirstClientError = BadRequest,
    LastClientError = 499,
    FirstServerError = InternalServerError,
    LastServerError = 599,

    Undefined = -1,
};


DECLARE_LOG_CATEGORY_EXTERN(LogHttpClient, Log, All);


DECLARE_DELEGATE_RetVal_TwoParams(bool, FShouldRetryDelegate, FHttpRequestPtr, FHttpResponsePtr);
DECLARE_DELEGATE_OneParam(FOnDebugMessageDelegate, const FString&);
DECLARE_DELEGATE_OneParam(FOnDriftDeprecationMessageDelegate, const FString&);


class ResponseContext
{
public:
    ResponseContext(const FHttpRequestPtr& _request, const FHttpResponsePtr& _response, const FDateTime& _sent, bool _successful)
    : request{ _request }
    , response{ _response }
    , responseCode{ _response.IsValid() ? _response->GetResponseCode() : -1 }
    , successful{ _successful }
    , sent{ _sent }
    , received{ FDateTime::UtcNow() }
    , errorHandled{ false }
    {}

    FHttpRequestPtr request;
    FHttpResponsePtr response;
    int32 responseCode;
    bool successful;
    FString message;
    FString error;
    FDateTime sent;
    FDateTime received;
    bool errorHandled;
};


DECLARE_DELEGATE_OneParam(FRequestErrorDelegate, ResponseContext&);
DECLARE_DELEGATE_OneParam(FUnhandledErrorDelegate, ResponseContext&);
DECLARE_DELEGATE_TwoParams(FResponseRecievedDelegate, ResponseContext&, JsonDocument&);
DECLARE_DELEGATE_OneParam(FProcessResponseDelegate, ResponseContext&);

DECLARE_DELEGATE_RetVal_TwoParams(bool, FDispatchRequestDelegate, TSharedRef<class HttpRequest>, bool);
DECLARE_DELEGATE_OneParam(FRequestCompletedDelegate, TSharedRef<class HttpRequest>);

    
class DRIFTHTTP_API HttpRequest : public TSharedFromThis<HttpRequest>
{
public:
    HttpRequest();

    virtual ~HttpRequest() {}

    /** Return the delegate to determine if the request should be retried */
    FShouldRetryDelegate& OnShouldRetry() { return shouldRetryDelegate_; }

    /** Return the delegate called when the progress of the request updates */
    FHttpRequestProgressDelegate& OnRequestProgress() { return wrappedRequest_->OnRequestProgress(); }

    /** Return the delegate called when the server returns debug headers */
    FOnDebugMessageDelegate& OnDebugMessage() { return onDebugMessage_; }

    /**
     * Dispatch the request for processing
     * The request may be sent over the wire immediately, or queued by the request manager
     * depending on how the manager is setup
     */
    bool Dispatch(bool forceQueued = false);

    /**
     * Mark request as discarded.
     * If the request has not yet been sent, it will be removed from the queue.
     * If it has been sent, the response will be ignored.
     */
    void Discard();
    /**
     * Destroy this request, and terminate any remaining processing where possible
     */
    void Destroy();
    
    void SetHeader(const FString& headerName, const FString& headerValue)
    {
        wrappedRequest_->SetHeader(headerName, headerValue);
    }

    void SetRetries(int32 retries) { retriesLeft_ = retries; }

    void SetContentType(const FString& contentType) { contentType_ = contentType; }
    
    void SetCache(TSharedPtr<IHttpCache> cache);
    
    FString GetAsDebugString() const;
    
    FRequestErrorDelegate OnError;
    FRequestErrorDelegate DefaultErrorHandler;
    FUnhandledErrorDelegate OnUnhandledError;
    FResponseRecievedDelegate OnResponse;
    FOnDriftDeprecationMessageDelegate OnDriftDeprecationMessage;

    FProcessResponseDelegate ProcessResponse;

    FDispatchRequestDelegate OnDispatch;
    FRequestCompletedDelegate OnCompleted;

#if !UE_BUILD_SHIPPING
    const FGuid& RequestID() const
    {
        return guid_;
    }
#endif
    
private:
    void BindActualRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> request);
    void InternalRequestCompleted(FHttpRequestPtr request, FHttpResponsePtr response, bool bWasSuccessful);
    void BroadcastError(ResponseContext& context);
    void LogError(ResponseContext& context);

protected:
    friend class RequestManager;

    /** Used by the request manager to set the payload */
    void SetPayload(const FString& content);

    /** Retry this request */
    void Retry();

    /** The actual http request object created by the engine */
    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> wrappedRequest_;

    /** Delegate called to determine if the request should be retried */
    FShouldRetryDelegate shouldRetryDelegate_;
    FOnDebugMessageDelegate onDebugMessage_;

    /** How many retries are left for this request */
    int32 retriesLeft_;

    FString contentType_;
    FDateTime sent_;

#if !UE_BUILD_SHIPPING
    FGuid guid_;
#endif

    int32 expectedResponseCode_;
    bool discarded_ = false;
    
    TSharedPtr<IHttpCache> cache_;
};


static FString GetDebugText(FHttpResponsePtr response)
{
    return FString::Printf(TEXT(" Response Code: %d\n Error Code: %d\n Text: %s"),
        response->GetResponseCode(),
        -1,// TODO: response->GetErrorCode(),
        *response->GetContentAsString()
    );
}
