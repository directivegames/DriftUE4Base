// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once


class CachedHttpResponse : public IHttpResponse
{
public:
    CachedHttpResponse();

    // IHttpBase API
    FString GetURL() override;
    FString GetURLParameter(const FString& ParameterName) override;
    FString GetHeader(const FString& HeaderName)  override;
    TArray<FString> GetAllHeaders()  override;
    FString GetContentType()  override;
    int32 GetContentLength()  override;
    const TArray<uint8>& GetContent() override;

    // IHttpResponse API
    int32 GetResponseCode() override;
    FString GetContentAsString() override;

    friend class FileHttpCache;

private:
    TMap<FString, FString> headers;
    TArray<uint8> payload;
    FString contentType;
    int32 responseCode;
    FString url;
};
