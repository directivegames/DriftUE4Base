// Copyright 2015-2019 Directive Games Limited - All Rights Reserved.

#pragma once

#include "JsonArchive.h"
#include "Interfaces/IHttpRequest.h"

#include "Core.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"


JSONARCHIVE_API DECLARE_LOG_CATEGORY_EXTERN(JsonUtilsLog, Log, All);


class JSONARCHIVE_API JsonUtils
{
public:
	template<class T>
	static bool ParseResponse(FHttpResponsePtr response, T& parsed)
	{
		bool success = ParseResponseNoLog(response, parsed);
		
		if (!success)
		{
			UE_LOG(JsonUtilsLog, Error, TEXT("Failed to parse json response '%s'"), *response->GetContentAsString());
		}
		
		return success;
	}
	
	template<class T>
	static bool ParseResponseNoLog(FHttpResponsePtr response, T& parsed)
	{
		const auto respStr = response->GetContentAsString();
		
		return JsonArchive::LoadObject(*respStr, parsed, false);
	}
};

