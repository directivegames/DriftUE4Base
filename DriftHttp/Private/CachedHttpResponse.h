// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "Interfaces/IHttpResponse.h"
#if WITH_ENGINE_VERSION_MACROS
    #include "Misc/EngineVersionComparison.h"
#endif // WITH_ENGINE_VERSION_MACROS


class CachedHttpResponse : public IHttpResponse
{
public:
    CachedHttpResponse();

#ifdef IS_CONST
    #error "Macro collision!"
#endif

#if WITH_ENGINE_VERSION_MACROS
    #if UE_VERSION_NEWER_THAN(4, 20, 0)
        #define IS_CONST const
    #endif // UE_VERSION_NEWER_THAN(4, 20, 0)
#else
    #define IS_CONST
#endif // WITH_ENGINE_VERSION_MACROS

    // IHttpBase API
    FString GetURL() IS_CONST override;
    FString GetURLParameter(const FString& ParameterName) IS_CONST override;
    FString GetHeader(const FString& HeaderName) IS_CONST  override;
    TArray<FString> GetAllHeaders() IS_CONST  override;
    FString GetContentType() IS_CONST  override;
    int32 GetContentLength() IS_CONST  override;
    const TArray<uint8>& GetContent() IS_CONST  override;

    // IHttpResponse API
    int32 GetResponseCode() IS_CONST  override;
    FString GetContentAsString() IS_CONST  override;

#undef IS_CONST

    friend class FileHttpCache;

private:
    TMap<FString, FString> headers;
    TArray<uint8> payload;
    FString contentType;
    int32 responseCode;
    FString url;
};
