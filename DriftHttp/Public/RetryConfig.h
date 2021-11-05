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

#pragma once


class HttpRequest;


class DRIFTHTTP_API FRetryConfig
{
public:
	FRetryConfig(int32 Retries);

	virtual ~FRetryConfig() = default;

	virtual void Apply(HttpRequest& Request) const;

protected:
	int32 GetRetries() const;
private:
	int32 Retries_;
};


class DRIFTHTTP_API FRetryOnServerError : public FRetryConfig
{
public:
	FRetryOnServerError();
	void Apply(HttpRequest& Request) const override;
};
