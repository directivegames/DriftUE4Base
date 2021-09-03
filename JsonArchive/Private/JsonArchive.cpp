/**
* This file is part of the Drift Unreal Engine Integration.
*
* Copyright (C) 2016-2019 Directive Games Limited. All Rights Reserved.
*
* Licensed under the MIT License (the "License");
*
* You may not use this file except in compliance with the License.
* You may obtain a copy of the license in the LICENSE file found at the top
* level directory of this module, and at https://mit-license.org/
*/


#include "JsonArchive.h"

#include "Json.h"
#include "Modules/ModuleManager.h"


IMPLEMENT_MODULE(FDefaultModuleImpl, JsonArchive)


template<>
bool JsonArchive::SerializeObject<int>(JsonValue& jValue, int32& cValue)
{
    bool success = false;

    if (isLoading_)
    {
        if (jValue.IsInt32())
        {
            cValue = jValue.GetInt32();
            success = true;
        }
    }
    else
    {
        jValue.SetInt32(cValue);
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
        if (jValue.IsInt32())
        {
            cValue = jValue.GetInt32();
            success = true;
        }
    }
    else
    {
        jValue.SetInt32(cValue);
        success = true;
    }

    return success;
}

template<>
bool JsonArchive::SerializeObject<unsigned>(JsonValue& jValue, uint32& cValue)
{
    bool success = false;

    if (isLoading_)
    {
        if (jValue.IsUint32())
        {
            cValue = jValue.GetUint32();
            success = true;
        }
    }
    else
    {
        jValue.SetUint32(cValue);
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
bool JsonArchive::SerializeObject<unsigned long long>(JsonValue& jValue, unsigned long long& cValue)
{
	bool success = false;

	if (isLoading_)
	{
		if (jValue.IsUint64())
		{
			cValue = jValue.GetUint64();
			success = true;
		}
	}
	else
	{
		jValue.SetUint64(cValue);
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
            cValue = static_cast<float>(jValue.GetDouble());
            success = true;
        }
        else if (jValue.IsInt32())
        {
            cValue = static_cast<float>(jValue.GetInt32());
            success = true;
        }
        else if (jValue.IsInt64())
        {
            cValue = static_cast<float>(jValue.GetInt64());
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
            cValue = static_cast<double>(jValue.GetDouble());
            success = true;
        }
        else if (jValue.IsInt32())
        {
            cValue = static_cast<double>(jValue.GetInt32());
            success = true;
        }
        else if (jValue.IsInt64())
        {
            cValue = static_cast<double>(jValue.GetInt64());
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
        jValue.SetString(*cValue);
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
            cValue = *jValue.GetString();
            success = true;
        }
    }
    else
    {
        FString temp;
        cValue.ToString(temp);
        jValue.SetString(*temp);
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
    	else if (jValue.IsNull())
    	{
    		cValue = FDateTime{ 0 };
    		success = true;
    	}
    }
    else
    {
        FString temp = cValue.ToIso8601();
        jValue.SetString(*temp);
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
        cValue.CopyFrom(jValue);
    }
    else
    {
        jValue.CopyFrom(cValue);
    }

    return true;
}

template<>
bool JsonArchive::SerializeObject<JsonValueWrapper>(JsonValue& jValue, JsonValueWrapper& cValue)
{
	return SerializeObject(jValue, cValue.value);
}


// use a strange string so that it won't conflict with other
static const FString VERSION_STRING(TEXT("$serialization_version"));


bool SerializationContext::IsLoading() const
{
	return archive.IsLoading();
}


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

	if (value.HasField(VERSION_STRING))
	{
		SerializeProperty(*VERSION_STRING, version);
	}

	return version;
}
