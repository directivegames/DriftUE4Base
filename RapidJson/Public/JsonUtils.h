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
