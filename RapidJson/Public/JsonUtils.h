// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "JsonArchive.h"
#include "IHttpRequest.h"

#include "Core.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"


RAPIDJSON_API DECLARE_LOG_CATEGORY_EXTERN(JsonUtilsLog, Log, All);


class RAPIDJSON_API JsonUtils
{
public:
    template<class T>
    static bool ParseResponse(FHttpResponsePtr response, T& parsed)
    {
        const auto respStr = response->GetContentAsString();

        bool success = JsonArchive::LoadObject(*respStr, parsed);

        if (!success)
        {
            UE_LOG(JsonUtilsLog, Error, TEXT("Failed to parse json response '%s'"), *respStr);
        }

        return success;
    }
};
