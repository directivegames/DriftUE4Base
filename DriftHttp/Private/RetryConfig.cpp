/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2021 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/

#include "RetryConfig.h"

#include "HttpRequest.h"


FRetryConfig::FRetryConfig(int32 Retries)
	: Retries_{ Retries }
{
}


void FRetryConfig::Apply(HttpRequest& Request) const
{
	Request.SetRetries(GetRetries());
}


int32 FRetryConfig::GetRetries() const
{
	return Retries_;
}


FRetryOnServerError::FRetryOnServerError()
	: FRetryConfig(3)
{
}


void FRetryOnServerError::Apply(HttpRequest& Request) const
{
	FRetryConfig::Apply(Request);
	Request.SetShouldRetryDelegate(FShouldRetryDelegate::CreateLambda([](FHttpRequestPtr Request, FHttpResponsePtr Response)
	{
		return Response.IsValid() && Response->GetResponseCode() >= EHttpResponseCodes::ServerError;
	}));
}
