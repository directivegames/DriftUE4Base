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

#include "JsonArchive.h"

using namespace rapidjson;

CrtAllocator JsonArchive::allocator_;

bool SerializationContext::IsLoading() const
{
    return archive.IsLoading();
}

template<>
bool JsonArchive::SerializeObject<int>(JsonValue& jValue, int& cValue)
{
    bool success = false;
    
    if (isLoading_)
    {
        if (jValue.IsInt())
        {
            cValue = jValue.GetInt();
            success = true;
        }
    }
    else
    {
        jValue.SetInt(cValue);
        success = true;
    }
    
    return success;
}

template<>
bool JsonArchive::SerializeObject<uint8>(JsonValue& jValue, uint8& cValue)
{
    bool success = false;
    
    if (isLoading_)
    {
        if (jValue.IsInt())
        {
            cValue = jValue.GetInt();
            success = true;
        }
    }
    else
    {
        jValue.SetInt(cValue);
        success = true;
    }
    
    return success;
}

template<>
bool JsonArchive::SerializeObject<unsigned>(JsonValue& jValue, unsigned& cValue)
{
    bool success = false;
    
    if (isLoading_)
    {
        if (jValue.IsUint())
        {
            cValue = jValue.GetUint();
            success = true;
        }
    }
    else
    {
        jValue.SetUint(cValue);
        success = true;
    }
    
    return success;
}

template<>
bool JsonArchive::SerializeObject<long long>(JsonValue& jValue, long long& cValue)
{
    bool success = false;
    
    if (isLoading_)
    {
        if (jValue.IsInt64())
        {
            cValue = jValue.GetInt64();
            success = true;
        }
    }
    else
    {
        jValue.SetInt64(cValue);
        success = true;
    }
    
    return success;
}

template<>
bool JsonArchive::SerializeObject<long>(JsonValue& jValue, long& cValue)
{
	bool success = false;

	if (isLoading_)
	{
		if (jValue.IsInt64())
		{
			cValue = jValue.GetInt64();
			success = true;
		}
	}
	else
	{
		jValue.SetInt64(cValue);
		success = true;
	}

	return success;
}

template<>
bool JsonArchive::SerializeObject<float>(JsonValue& jValue, float& cValue)
{
    bool success = false;

    if (isLoading_)
    {
        if (jValue.IsDouble())
        {
            cValue = (float)jValue.GetDouble();
            success = true;
        }
        else if (jValue.IsInt())
        {
            cValue = (float)jValue.GetInt();
            success = true;
        }
        else if (jValue.IsInt64())
        {
            cValue = (float)jValue.GetInt64();
            success = true;
        }
    }
    else
    {
        jValue.SetDouble(cValue);
        success = true;
    }

    return success;
}

template<>
bool JsonArchive::SerializeObject<double>(JsonValue& jValue, double& cValue)
{
    bool success = false;
    
    if (isLoading_)
    {
        if (jValue.IsDouble())
        {
            cValue = (double)jValue.GetDouble();
            success = true;
        }
        else if (jValue.IsInt())
        {
            cValue = (double)jValue.GetInt();
            success = true;
        }
        else if (jValue.IsInt64())
        {
            cValue = (double)jValue.GetInt64();
            success = true;
        }
    }
    else
    {
        jValue.SetDouble(cValue);
        success = true;
    }
    
    return success;
}

template<>
bool JsonArchive::SerializeObject<bool>(JsonValue& jValue, bool& cValue)
{
    bool success = false;

    if (isLoading_)
    {
        if (jValue.IsBool())
        {
            cValue = jValue.GetBool();
            success = true;
        }
    }
    else
    {
        jValue.SetBool(cValue);
        success = true;
    }

    return success;
}

template<>
bool JsonArchive::SerializeObject<FString>(JsonValue& jValue, FString& cValue)
{
    bool success = false;

    if (isLoading_)
    {
        if (jValue.IsString())
        {
            cValue = jValue.GetString();
            success = true;
        }
        else if (jValue.IsNull())
        {
            cValue = TEXT("");
            success = true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        jValue.SetString(*cValue, allocator_);
        success = true;
    }

    return success;
}

template<>
bool JsonArchive::SerializeObject<FName>(JsonValue& jValue, FName& cValue)
{
    bool success = false;
    
    if (isLoading_)
    {
        if (jValue.IsString())
        {
            cValue = jValue.GetString();
            success = true;
        }
    }
    else
    {
        FString temp;
        cValue.ToString(temp);
        jValue.SetString(*temp, allocator_);
        success = true;
    }
    
    return success;
}

template<>
bool JsonArchive::SerializeObject<FDateTime>(JsonValue& jValue, FDateTime& cValue)
{
    bool success = false;
    
    if (isLoading_)
    {
        if (jValue.IsString())
        {
            FString temp = jValue.GetString();
            // Date only
            if (temp.Right(1) == TEXT("Z") && !temp.Contains(TEXT("T")))
            {
                temp = temp.LeftChop(1);
                success = FDateTime::ParseIso8601(*temp, cValue);
            }
            // FDateTime refuses to accept more than 3 digits of sub-second resolution
            else if (temp.Right(1) == TEXT("Z") || (temp.Contains(TEXT("T")) && temp.Right(1).IsNumeric()))
            {
                int32 millisecondPeriod;
                if (temp.FindLastChar(L'.', millisecondPeriod))
                {
                    // period plus 3 digits
                    temp = temp.Left(millisecondPeriod + 4) + TEXT("Z");
                    success = FDateTime::ParseIso8601(*temp, cValue);
                }
            }
        }
    }
    else
    {
        FString temp = cValue.ToIso8601();
        jValue.SetString(*temp, allocator_);
        success = true;
    }
    
    return success;
}

template<>
bool JsonArchive::SerializeObject<FTimespan>(JsonValue& jValue, FTimespan& cValue)
{
    bool success = false;
    
    if (isLoading_)
    {
        if (jValue.IsInt64())
        {
            cValue = FTimespan(jValue.GetInt64());
            success = true;
        }
    }
    else
    {
        jValue.SetInt64(cValue.GetTicks());
        success = true;
    }
    
    return success;
}

template<>
bool JsonArchive::SerializeObject<JsonValue>(JsonValue& jValue, JsonValue& cValue)
{
    if (isLoading_)
    {
        cValue.CopyFrom(jValue, allocator_);
    }
    else
    {
        jValue.CopyFrom(cValue, allocator_);
    }

    return true;
}

template<>
bool JsonArchive::SerializeObject<JsonValueWrapper>(JsonValue& jValue, JsonValueWrapper& cValue)
{
	return SerializeObject(jValue, cValue.value);
}

JsonValueWrapper::JsonValueWrapper(const JsonValueWrapper& other)
{
	value.CopyFrom(other.value, JsonArchive::Allocator());
}

JsonValueWrapper::JsonValueWrapper(JsonValueWrapper&& other) :
	value(std::move(other.value))
{
}

JsonValueWrapper::JsonValueWrapper(const JsonValue& other)
{
	value.CopyFrom(other, JsonArchive::Allocator());
}

JsonValueWrapper::JsonValueWrapper(JsonValue&& other) :
	value(std::move(other))
{
}

JsonValueWrapper& JsonValueWrapper::operator=(const JsonValueWrapper& other)
{
	if (this != &other)
	{
		value.CopyFrom(other.value, JsonArchive::Allocator());
	}

	return *this;
}

JsonValueWrapper& JsonValueWrapper::operator=(JsonValueWrapper&& other)
{
	if (this != &other)
	{
		value = std::move(other.value);
	}

	return *this;
}

// use a strange sting so that it won't conflict with other
static const FString VERSION_STRING(TEXT("$serialization_version"));

void SerializationContext::SetVersion(int version)
{
	if (!IsLoading())
	{
		SerializeProperty(*VERSION_STRING, version);
	}
}

int SerializationContext::GetVersion()
{
	int version = -1;

	if (value.HasMember(*VERSION_STRING))
	{
		SerializeProperty(*VERSION_STRING, version);
	}

	return version;
}
