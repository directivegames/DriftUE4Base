// Copyright 2016-2017 Directive Games Limited - All Rights Reserved


#include "CachedHttpResponse.h"

#include "Containers/StringConv.h"


#ifdef WITH_ENGINE_VERSION_MACROS
    #if UE_VERSION_NEWER_THAN(4, 20, 0)
        #define IS_CONST const
    #endif // UE_VERSION_NEWER_THAN(4, 20, 0)
#else
    #define IS_CONST
#endif // WITH_ENGINE_VERSION_MACROS


CachedHttpResponse::CachedHttpResponse()
{
    
}


FString CachedHttpResponse::GetURL() IS_CONST
{
    return url;
}


FString CachedHttpResponse::GetURLParameter(const FString &ParameterName) IS_CONST
{
    return TEXT("");
}


FString CachedHttpResponse::GetHeader(const FString &HeaderName) IS_CONST
{
    const auto header = headers.Find(HeaderName);
    if (header != nullptr)
    {
        return *header;
    }
    return TEXT("");
}


TArray<FString> CachedHttpResponse::GetAllHeaders() IS_CONST
{
    TArray<FString> result;
    for (const auto& header : headers)
    {
        result.Add(header.Key + TEXT(": ") + header.Value);
    }
    return result;
}


FString CachedHttpResponse::GetContentType() IS_CONST
{
    return contentType;
}


int32 CachedHttpResponse::GetContentLength() IS_CONST
{
    return payload.Num();
}


const TArray<uint8>& CachedHttpResponse::GetContent() IS_CONST
{
    return payload;
}


int32 CachedHttpResponse::GetResponseCode() IS_CONST
{
    return responseCode;
}


FString CachedHttpResponse::GetContentAsString() IS_CONST
{
    TArray<uint8> zeroTerminatedPayload(GetContent());
    zeroTerminatedPayload.Add(0);
    return UTF8_TO_TCHAR(zeroTerminatedPayload.GetData());
}
