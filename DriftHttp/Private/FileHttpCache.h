// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once


#include "DriftHttpCache.h"
#include "HttpCacheEntry.h"


class DRIFTHTTP_API FileHttpCache : public IHttpCache
{
public:
    FileHttpCache();

    void CacheResponse(const ResponseContext& context) override;
    FHttpResponsePtr GetCachedResponse(const FString& url) override;

private:
    void LoadCache();
    void SaveIndex();
    
    FString MakeEntryPath(const FString& name) const;

    bool LoadResponse(const FString& name, HttpCacheEntry& entry);
    bool SaveResponse(const FString& name, const HttpCacheEntry& entry);
    bool LoadBody(const FString& name, TArray<uint8>& body);
    bool SaveBody(const FString& name, const TArray<uint8>& body);

    FString GetContentHash(const FHttpResponsePtr& response) const;
    FTimespan CalculateCorrectedInitialAge(const FHttpResponsePtr& response, HttpCacheEntry& entry) const;
    FHttpResponsePtr MakeResponse(HttpCacheEntry& entry);

    FString cacheDir;
    TMap<FString, HttpCacheEntry> data;
    TMap<FString, FString> index;
    
    int32 cacheVersion;
};
