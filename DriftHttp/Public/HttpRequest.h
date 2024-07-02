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

#pragma once

#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonArchive.h"
#include "Misc/EngineVersionComparison.h"


class IHttpCache;
class FRetryConfig;


// based on http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
enum class HttpStatusCodes
{
	Ok = 200
	, Created = 201
	, Accepted = 202
	, NoContent = 204
	, Moved = 301
	, Found = 302
	, SeeOther = 303
	, NotModified = 304
	, BadRequest = 400
	, Unauthorized = 401
	, Forbidden = 403
	, NotFound = 404
	, NotAllowed = 405
	, NotAcceptable = 406
	, Timeout = 408
	, InternalServerError = 500
	, NotImplemented = 501
	, BadGateway = 502
	, ServiceUnavailable = 503
	, GatewayTimeout = 504
	, FirstClientError = BadRequest
	, LastClientError = 499
	, FirstServerError = InternalServerError
	, LastServerError = 599
	, Undefined = -1
	,
};


DECLARE_LOG_CATEGORY_EXTERN(LogHttpClient, Log, All);


DECLARE_DELEGATE_RetVal_TwoParams(bool, FShouldRetryDelegate, FHttpRequestPtr, FHttpResponsePtr);
DECLARE_DELEGATE_OneParam(FOnDebugMessageDelegate, const FString&);
DECLARE_DELEGATE_OneParam(FOnDriftDeprecationMessageDelegate, const FString&);


class ResponseContext
{
public:
	ResponseContext(const FHttpRequestPtr& _request, const FHttpResponsePtr& _response, const FDateTime& _sent
	                , bool _successful)
		: request{ _request }
		, response{ _response }
		, responseCode{ _response.IsValid() ? _response->GetResponseCode() : -1 }
		, successful{ _successful }
		, sent{ _sent }
		, received{ FDateTime::UtcNow() }
		, errorHandled{ false }
	{
	}


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

DECLARE_DELEGATE_RetVal_OneParam(bool, FDispatchRequestDelegate, TSharedRef<class HttpRequest>);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FRetryRequestDelegate, TSharedRef<class HttpRequest>, float);
DECLARE_DELEGATE_OneParam(FRequestCompletedDelegate, TSharedRef<class HttpRequest>);


class DRIFTHTTP_API HttpRequest : public TSharedFromThis<HttpRequest>
{
public:
	HttpRequest();


	virtual ~HttpRequest()
	{
	}


	/** Return the delegate to determine if the request should be retried */
	FShouldRetryDelegate& OnShouldRetry() { return shouldRetryDelegate_; }

	/** Return the delegate called when the progress of the request updates */
#if UE_VERSION_OLDER_THAN(5, 4, 0)
	FHttpRequestProgressDelegate& OnRequestProgress() { return wrappedRequest_->OnRequestProgress(); }
#else
    FHttpRequestProgressDelegate64& OnRequestProgress() { return wrappedRequest_->OnRequestProgress64(); }
#endif

	/** Return the delegate called when the server returns debug headers */
	FOnDebugMessageDelegate& OnDebugMessage() { return onDebugMessage_; }

	/**
	 * Dispatch the request for processing
	 * The request may be sent over the wire immediately, or queued by the request manager
	 * depending on how the manager is setup
	 */
	bool Dispatch();
	bool EnqueueWithDelay(float Delay);

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


	void SetRetries(int32 retries) { MaxRetries_ = retries; }
	void SetRetryConfig(const FRetryConfig& Config);

	void SetContentType(const FString& contentType) { contentType_ = contentType; }

	void SetContent(const TArray<uint8>& ContentPayload);

	void SetCache(TSharedPtr<IHttpCache> cache);

	void SetShouldRetryDelegate(FShouldRetryDelegate Delegate) { shouldRetryDelegate_ = Delegate; }

	void SetExpectJsonResponse(bool expectJsonResponse) { expectJsonResponse_ = expectJsonResponse; }

	/** Used by the request manager to set the payload */
	void SetPayload(const FString& content);

	FString GetAsDebugString(bool detailed = false) const;

	FString GetRequestURL() const;

	FString GetContentAsString() const;

	FRequestErrorDelegate OnError;
	FRequestErrorDelegate DefaultErrorHandler;
	FUnhandledErrorDelegate OnUnhandledError;
	FResponseRecievedDelegate OnResponse;
	FOnDriftDeprecationMessageDelegate OnDriftDeprecationMessage;

	FProcessResponseDelegate ProcessResponse;

	FDispatchRequestDelegate OnDispatch;
	FRetryRequestDelegate OnRetry;
	FRequestCompletedDelegate OnCompleted;

#if !UE_BUILD_SHIPPING
	const FGuid& RequestID() const
	{
		return guid_;
	}
#endif

private:
	void BindActualRequest(FHttpRequestPtr request);
	void InternalRequestCompleted(FHttpRequestPtr request, FHttpResponsePtr response, bool bWasSuccessful);
	void BroadcastError(ResponseContext& context);
	void LogError(ResponseContext& context);

protected:
	friend class RequestManager;

	/** Retry this request */
	void Retry();

	/** The actual http request object created by the engine */
	FHttpRequestPtr wrappedRequest_;

	/** Delegate called to determine if the request should be retried */
	FShouldRetryDelegate shouldRetryDelegate_;
	FOnDebugMessageDelegate onDebugMessage_;

	/** How many retries are left for this request */
	int32 MaxRetries_;

	/** Current retry attempt, used to calculate back-off */
	int32 CurrentRetry_;

	float RetryDelay_ = 1.0f;
	float RetryDelayCap_ = 10.0f;

	FString contentType_;
	FDateTime sent_;

#if !UE_BUILD_SHIPPING
	FGuid guid_;
#endif

	int32 expectedResponseCode_;
	bool discarded_ = false;
	bool expectJsonResponse_ = true;

	TSharedPtr<IHttpCache> cache_;
};


static FString GetDebugText(FHttpResponsePtr response)
{
	return FString::Printf(TEXT(" Response Code: %d\n Error Code: %d\n Text: %s"),
	                       response->GetResponseCode(),
	                       -1, // TODO: response->GetErrorCode(),
	                       *response->GetContentAsString()
	);
}

// A fake response whose only purpose is to provide a valid response object when null is returned from the engine
class DRIFTHTTP_API FFakeHttpResponse : public IHttpResponse
{
public:
    FFakeHttpResponse(const FString& url, int32 responseCode, const FString& content);

    // IHttpResponse
    int32 GetResponseCode() const override;
    FString GetContentAsString() const override;
    // !IHttpResponse

    // IHttpBase
    FString GetURL() const override;
    FString GetURLParameter(const FString& ParameterName) const override;
    FString GetHeader(const FString& HeaderName) const override;
    TArray<FString> GetAllHeaders() const override;
    FString GetContentType() const override;
    uint64 GetContentLength() const override;
    const TArray<uint8>& GetContent() const override;

#if !UE_VERSION_OLDER_THAN(5, 4, 0)
    const FString& GetEffectiveURL() const override { return url_; }
    EHttpRequestStatus::Type GetStatus() const override { return EHttpRequestStatus::Failed; }
    EHttpFailureReason GetFailureReason() const override { return EHttpFailureReason::ConnectionError; }
#endif
    // !IHttpBase

private:
    FString url_;
    int32 responseCode_;
    FString content_;
    TArray<uint8> contentBytes_;
};
