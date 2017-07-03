// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#include "DriftHttpPCH.h"

#include "CachedHttpResponse.h"


CachedHttpResponse::CachedHttpResponse()
{
    
}


FString CachedHttpResponse::GetURL()
{
    return url;
}


FString CachedHttpResponse::GetURLParameter(const FString &parameterName)
{
    return TEXT("");
}


FString CachedHttpResponse::GetHeader(const FString &headerName)
{
    auto header = headers.Find(headerName);
    if (header != nullptr)
    {
        return *header;
    }
    return TEXT("");
}


TArray<FString> CachedHttpResponse::GetAllHeaders()
{
    TArray<FString> result;
    for (const auto& header : headers)
    {
        result.Add(header.Key + TEXT(": ") + header.Value);
    }
    return result;
}


FString CachedHttpResponse::GetContentType()
{
    return contentType;
}


int32 CachedHttpResponse::GetContentLength()
{
    return payload.Num();
}


const TArray<uint8>& CachedHttpResponse::GetContent()
{
    return payload;
}


int32 CachedHttpResponse::GetResponseCode()
{
    return responseCode;
}


FString CachedHttpResponse::GetContentAsString()
{
    TArray<uint8> zeroTerminatedPayload(GetContent());
    zeroTerminatedPayload.Add(0);
    return UTF8_TO_TCHAR(zeroTerminatedPayload.GetData());
}
