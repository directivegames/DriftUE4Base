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


#include "HttpRequest.h"

#include "Http.h"
#include "HttpCache.h"
#include "JsonArchive.h"
#include "JsonUtils.h"
#include "ErrorResponse.h"
#include "IErrorReporter.h"
#include "RetryConfig.h"


#define LOCTEXT_NAMESPACE "Drift"


HttpRequest::HttpRequest()
	: MaxRetries_{ 0 }
	, CurrentRetry_{ 0 }
	, sent_{ FDateTime::UtcNow() }
	, expectedResponseCode_{ static_cast<int32>(HttpStatusCodes::Ok) }
{
#if !UE_BUILD_SHIPPING
	FPlatformMisc::CreateGuid(guid_);
#endif
}


void HttpRequest::SetContent(const TArray<uint8>& ContentPayload)
{
	wrappedRequest_->SetContent(ContentPayload);
}

void HttpRequest::SetCache(TSharedPtr<IHttpCache> cache)
{
	cache_ = cache;
}


void HttpRequest::BindActualRequest(FHttpRequestPtr request)
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

    check(request);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	if (response.IsValid())
	{
		/**
		 * Grab out-of-band debug messages from http response header and display
		 * it to the player.
		 */
		auto debug_message = response->GetHeader(TEXT("Drift-Debug-Message"));
		if (debug_message.Len())
		{
			OnDebugMessage().ExecuteIfBound(debug_message);
		}
	}
#endif

    if (!response)
    {
        if (bWasSuccessful)
        {
            ensureAlwaysMsgf(false, TEXT("Did not get a valid response from a successful request to '%s'"), *request->GetURL());
        }
        response = MakeShared<FFakeHttpResponse>(request->GetURL(), INDEX_NONE, TEXT("This is a fake response since the engine/OS returns null"));
    }

	ResponseContext context(request, response, sent_, false);

#if UE_VERSION_OLDER_THAN(5, 4, 0)
    const auto bWasConnectionError = request->GetStatus() == EHttpRequestStatus::Failed_ConnectionError;
#else
	const auto bWasConnectionError = (request->GetStatus() == EHttpRequestStatus::Failed && request->GetFailureReason() == EHttpFailureReason::ConnectionError);
#endif
	if (!bWasConnectionError && response.IsValid())
	{
		/**
		 * We got a response from the server, let's check if it's good or bad
		 */
		const auto bStatusCodeIsSuccess = context.responseCode >= static_cast<int32>(HttpStatusCodes::Ok)
			&& context.responseCode < static_cast<int32>(HttpStatusCodes::FirstClientError);
		if (bStatusCodeIsSuccess)
		{
			/**
			 * We got a non-error response code
			 */
			if (expectJsonResponse_)
			{
				const auto contentType = context.response->GetHeader(TEXT("Content-Type"));
				if (!contentType.StartsWith(TEXT("application/json")))
					// to handle cases like `application/json; charset=UTF-8`
				{
					context.error = FString::Printf(
						TEXT("Expected Content-Type 'application/json', but got '%s'"), *contentType);
				}
				else
				{
					JsonDocument doc;
					FString content = response->GetContentAsString();
					if (context.responseCode == static_cast<int32>(HttpStatusCodes::NoContent))
					{
						content = TEXT("{}");
					}
					doc.Parse(*content);
					if (doc.HasParseError())
					{
						context.error = FString::Printf(
							TEXT("JSON response is broken at position %i. RapidJson error: %i"),
							static_cast<int32>(doc.GetErrorOffset()),
							static_cast<int32>(doc.GetParseError()));
					}
					else if (expectedResponseCode_ != -1 && context.responseCode != expectedResponseCode_)
					{
						context.error = FString::Printf(
							TEXT("Expected '%i', but got '%i'"), expectedResponseCode_, context.responseCode);
						if (doc.HasField(TEXT("message")))
						{
							context.message = doc[TEXT("message")].GetString();
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
						const auto deprecationHeader = response->GetHeader(TEXT("Drift-Feature-Deprecation"));
						if (!deprecationHeader.IsEmpty())
						{
							OnDriftDeprecationMessage.ExecuteIfBound(deprecationHeader);
						}
					}
				}
			}
			else
			{
				// Non-Json response code path
				if (expectedResponseCode_ != -1 && context.responseCode != expectedResponseCode_)
				{
					context.error = FString::Printf(
						TEXT("Expected '%i', but got '%i'"), expectedResponseCode_, context.responseCode);
				}
				else
				{
					if (cache_.IsValid() && request->GetVerb() == TEXT("GET"))
					{
						cache_->CacheResponse(context);
					}

					context.successful = true;
					JsonDocument doc;
					OnResponse.ExecuteIfBound(context, doc);
				}
			}
			/**
			 * If the error is set, but also handled, that means the caller dealt with it.
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
				UE_LOG(LogHttpClient, Verbose, TEXT("'%s' SUCCEEDED in %.3f seconds"), *GetAsDebugString()
				       , (FDateTime::UtcNow() - sent_).GetTotalSeconds());
			}
		}
		else
		{
			if (CurrentRetry_ < MaxRetries_ && shouldRetryDelegate_.IsBound() && shouldRetryDelegate_.Execute(
				request, response))
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
				if (!context.errorHandled)
				{
					LogError(context);
				}
			}
		}
	}
	else
	{
		/**
		 * The request failed to send, or return. Pass it through the error handling chain.
		 */
		BroadcastError(context);
		if (!context.errorHandled)
		{
			LogError(context);
		}
	}

	OnCompleted.ExecuteIfBound(SharedThis(this));
}


void HttpRequest::BroadcastError(ResponseContext& context)
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
	error->SetNumberField(TEXT("elapsed"), (FDateTime::UtcNow() - sent_).GetTotalSeconds());
	error->SetBoolField(TEXT("error_handled"), context.errorHandled);
	error->SetNumberField(TEXT("status_code"), context.responseCode);
	if (!context.message.IsEmpty())
	{
		error->SetStringField(TEXT("message"), context.message);
	}
	if (!context.error.IsEmpty())
	{
		error->SetStringField(TEXT("error"), context.error);
	}

	auto requestData = MakeShared<FJsonObject>();
	requestData->SetStringField(TEXT("method"), wrappedRequest_->GetVerb());
	requestData->SetStringField(TEXT("url"), wrappedRequest_->GetURL());


	auto requestHeaders = context.request->GetAllHeaders();
	if (requestHeaders.Num() > 0)
	{
		auto headers = MakeShared<FJsonObject>();
		for (const auto& header : requestHeaders)
		{
			FString key, value;
			if (header.Split(TEXT(": "), &key, &value))
			{
				headers->SetStringField(key, value);
			}
		}
		requestData->SetObjectField(TEXT("headers"), headers);
	}

	{
		auto zeroTerminatedPayload{ context.request->GetContent() };
		zeroTerminatedPayload.Add(0);
		FString payload{ UTF8_TO_TCHAR(zeroTerminatedPayload.GetData()) };
		if (payload.Len() < 1024)
		{
			requestData->SetStringField(TEXT("data"), payload);
		}
		else if (payload.Len() > 0)
		{
			requestData->SetStringField(TEXT("data"), TEXT("[Truncated]"));
		}
	}

	if (context.response.IsValid())
	{
		GenericRequestErrorResponse response;
		if (JsonUtils::ParseResponseNoLog(context.response, response))
		{
			auto code = response.GetErrorCode();
			if (!code.IsEmpty())
			{
				error->SetStringField(TEXT("response_code"), code);
				errorMessage += code;
			}
			auto reason = response.GetErrorReason();
			if (!reason.IsEmpty() && reason != TEXT("undefined"))
			{
				error->SetStringField(TEXT("reason"), reason);
				errorMessage += errorMessage.IsEmpty() ? reason : FString{ TEXT(" : ") } + reason;
			}
			auto description = response.GetErrorDescription();
			if (!description.IsEmpty())
			{
				error->SetStringField(TEXT("description"), description);
			}
		}

		auto content = context.response->GetContentAsString();
		if (content.Len() < 1024)
		{
			error->SetStringField(TEXT("response_data"), content);
		}
		else if (content.Len() > 0)
		{
			error->SetStringField(TEXT("response_data"), TEXT("[Truncated]"));
		}

		auto responseHeaders = context.response->GetAllHeaders();
		if (responseHeaders.Num() > 0)
		{
			auto headers = MakeShared<FJsonObject>();
			for (const auto& header : responseHeaders)
			{
				FString key, value;
				if (header.Split(TEXT(": "), &key, &value))
				{
					headers->SetStringField(key, value);
				}
			}
			error->SetObjectField(TEXT("response_headers"), headers);
		}
	}
	else
	{
#if UE_VERSION_OLDER_THAN(5, 4, 0)
		if (context.request->GetStatus() == EHttpRequestStatus::Failed_ConnectionError)
#else
        if (context.request->GetStatus() == EHttpRequestStatus::Failed && context.request->GetFailureReason() == EHttpFailureReason::ConnectionError)
#endif
		{
			errorMessage = TEXT("HTTP request timeout");
		}
	}

	if (errorMessage.IsEmpty())
	{
		/**
		 * This pattern cannot be static, some internal smart pointer will crash on shutdown.
		 * Since it's only used when there's an error, the cost of on-demand creation is acceptable.
		 */
		const FRegexPattern urlNormalizationPattern{ TEXT(".*?[/=]+([0-9]+)[&?/=]?.*") };

		FString normalizedUrl = wrappedRequest_->GetURL();
		TArray<TSharedPtr<FJsonValue>> params;

		int index = 0;
		FRegexMatcher matcher{ urlNormalizationPattern, normalizedUrl };
		while (matcher.FindNext())
		{
			normalizedUrl = FString::Printf(TEXT("%s{%d}%s"), *normalizedUrl.Left(matcher.GetCaptureGroupBeginning(1))
			                                , index, *normalizedUrl.Mid(matcher.GetCaptureGroupEnding(1)));
			params.Add(MakeShared<FJsonValueString>(matcher.GetCaptureGroup(1)));
			matcher = FRegexMatcher{ urlNormalizationPattern, normalizedUrl };
			++index;
		}

		errorMessage = FString::Printf(TEXT("HTTP request failed: %s %s"), *wrappedRequest_->GetVerb(), *normalizedUrl);

		if (params.Num() > 0)
		{
			error->SetField(TEXT("params"), MakeShared<FJsonValueArray>(params));
		}
	}
	error->SetObjectField(TEXT("request"), requestData);
	IErrorReporter::Get()->AddError(TEXT("LogHttpClient"), *errorMessage, error);
}


void HttpRequest::Retry()
{
	++CurrentRetry_;

	const auto MaxRetryDelay = FMath::Min(RetryDelayCap_, FMath::Pow(RetryDelay_ * 2, CurrentRetry_));
	const auto Delay = FMath::RandRange(RetryDelay_ / 2.0f, MaxRetryDelay);

	UE_LOG(LogHttpClient, Verbose, TEXT("Scheduling retry for %s in %f seconds"), *GetAsDebugString(), Delay);

	// Note that we explicitly set the request to be queued for retry
	// the reason is that the internal HTTP processing logic will remove the current request from the system right after this point (Retry is called by the request finish handler)
	// so the only way to make it work is to add the request back in the next tick (this is done automatically by the queue in the request manager)
	EnqueueWithDelay(Delay);
}


bool HttpRequest::Dispatch()
{
	check(!wrappedRequest_->GetURL().IsEmpty());

	if (cache_.IsValid() && wrappedRequest_->GetVerb() == TEXT("GET"))
	{
		const auto header = wrappedRequest_->GetHeader(TEXT("Cache-Control"));
		if (!(header.Contains(TEXT("no-cache")) || header.Contains(TEXT("max-age=0"))))
		{
			const auto cachedResponse = cache_->GetCachedResponse(wrappedRequest_->GetURL());
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
					UE_LOG(LogHttpClient, Verbose, TEXT("'%s' SUCCEEDED from CACHE in %.3f seconds")
					       , *GetAsDebugString(), (FDateTime::UtcNow() - sent_).GetTotalSeconds());
				}

				return true;
			}
		}
	}

	if (OnDispatch.IsBound())
	{
		return OnDispatch.Execute(SharedThis(this));
	}
	return false;
}


bool HttpRequest::EnqueueWithDelay(float Delay)
{
	check(!wrappedRequest_->GetURL().IsEmpty());

	if (OnRetry.IsBound())
	{
		return OnRetry.Execute(SharedThis(this), Delay);
	}
	return false;
}


void HttpRequest::Discard()
{
	discarded_ = true;

    OnRequestProgress().Unbind();
}


void HttpRequest::Destroy()
{
	Discard();

	wrappedRequest_->CancelRequest();
}


FString HttpRequest::GetRequestURL() const
{
	return *wrappedRequest_->GetURL();
}


void HttpRequest::SetRetryConfig(const FRetryConfig& Config)
{
	Config.Apply(*this);
}


FString HttpRequest::GetAsDebugString(bool detailed) const
{
#if !UE_BUILD_SHIPPING
	FString ret = FString::Printf(TEXT("Http Request(%s): %s - %s"), *guid_.ToString(), *wrappedRequest_->GetVerb()
	                              , *wrappedRequest_->GetURL());
	if (detailed)
	{
		ret += LINE_TERMINATOR TEXT("Headers:") LINE_TERMINATOR;
		for (auto& header : wrappedRequest_->GetAllHeaders())
		{
			ret += header + LINE_TERMINATOR;
		}
		auto body = GetContentAsString();
		if (body.Len() > 0)
		{
			ret += TEXT("Body:") LINE_TERMINATOR;
			ret += body;
		}
	}
	return ret;
#else
    return FString::Printf(TEXT("Http Request: %s - %s"), *wrappedRequest_->GetVerb(), *wrappedRequest_->GetURL());
#endif
}


FString HttpRequest::GetContentAsString() const
{
	auto zeroTerminatedPayload(wrappedRequest_->GetContent());
	zeroTerminatedPayload.Add(0);
	return UTF8_TO_TCHAR(zeroTerminatedPayload.GetData());
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


FFakeHttpResponse::FFakeHttpResponse(const FString& url, int32 responseCode, const FString& content)
    : url_{ url }
    , responseCode_{ responseCode }
    , content_{ content }
{
    auto Converted = StringCast<UTF8CHAR>(*content);
    contentBytes_.Append((const uint8*)Converted.Get(), Converted.Length());
}


int32 FFakeHttpResponse::GetResponseCode() const
{
    return responseCode_;
}


FString FFakeHttpResponse::GetContentAsString() const
{
    return content_;
}


FString FFakeHttpResponse::GetURL() const
{
    return url_;
}


FString FFakeHttpResponse::GetURLParameter(const FString& ParameterName) const
{
    return {};
}


FString FFakeHttpResponse::GetHeader(const FString& HeaderName) const
{
    return {};
}


TArray<FString> FFakeHttpResponse::GetAllHeaders() const
{
    return {};
}


FString FFakeHttpResponse::GetContentType() const
{
    return TEXT("text/plain");
}


uint64 FFakeHttpResponse::GetContentLength() const
{
    return contentBytes_.Num();
}


const TArray<uint8>& FFakeHttpResponse::GetContent() const
{
    return contentBytes_;
}

#undef LOCTEXT_NAMESPACE
