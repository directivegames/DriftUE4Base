// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once
#include "Http.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHttpCache, Log, All);


class ResponseContext;


class IHttpCache
{
public:
    virtual void CacheResponse(const ResponseContext& context) = 0;
    virtual FHttpResponsePtr GetCachedResponse(const FString& url) = 0;
    
    virtual ~IHttpCache() {}
};


class IHttpCacheFactory
{
public:
    virtual TSharedPtr<IHttpCache> Create() = 0;

    virtual ~IHttpCacheFactory() {}
};
