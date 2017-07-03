// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "JsonArchive.h"


struct GenericRequestErrorResponse
{
    int32 status_code;
    FString message;
    FString errorText;
    
    const FString errorName{ TEXT("error") };
    
    FString GetErrorCode() const { return GetErrorString(TEXT("code")); }
    FString GetErrorReason() const { return GetErrorString(TEXT("reason")); }
    FString GetErrorDescription() const { return GetErrorString(TEXT("description")); }
    FString GetErrorContextID() const { return GetErrorString(TEXT("context_id")); }
    FString GetErrorLink() const { return GetErrorString(TEXT("link")); }
    
    FString GetErrorString(const FString& name) const
    {
        if (!error.IsNull() && error.HasMember(*name) && error[*name].IsString())
        {
            return FString(error[*name].GetString());
        }
        return TEXT("undefined");
    }
    
    int GetErrorInt(const FString& name) const
    {
        if (!error.IsNull() && error.HasMember(*name) && error[*name].IsInt())
        {
            return error[*name].GetInt();
        }
        return -1;
    }
    
    bool Serialize(SerializationContext& context)
    {
        bool result = true;
        if (context.IsLoading())
        {
            result &= context.SerializeProperty(TEXT("status_code"), status_code);
            result &= context.SerializeProperty(TEXT("message"), message);
            if (context.GetValue().HasMember(*errorName))
            {
                JsonValue& errorValue = context.GetValue()[*errorName];
                if (errorValue.IsObject())
                {
                    result &= context.SerializeProperty(*errorName, error);
                }
                else if (errorValue.IsString())
                {
                    context.SerializeProperty(*errorName, errorText);
                }
            }
        }
        return result;
    }
    
private:
    JsonValue error;
};
