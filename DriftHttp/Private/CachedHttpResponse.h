// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once


class CachedHttpResponse : public IHttpResponse
{
public:
    CachedHttpResponse();

    // IHttpBase API
    FString GetURL() const override;
    FString GetURLParameter(const FString& ParameterName) const override;
    FString GetHeader(const FString& HeaderName)  const override;
    TArray<FString> GetAllHeaders()  const override;
    FString GetContentType()  const override;
    int32 GetContentLength()  const override;
    const TArray<uint8>& GetContent() const override;

    // IHttpResponse API
    int32 GetResponseCode() const override;
    FString GetContentAsString() const override;

    friend class FileHttpCache;

private:
    TMap<FString, FString> headers;
    TArray<uint8> payload;
    FString contentType;
    int32 responseCode;
    FString url;
};
