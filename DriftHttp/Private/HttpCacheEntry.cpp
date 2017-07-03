// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#include "DriftHttpPCH.h"

#include "HttpCacheEntry.h"
#include "JsonArchive.h"


int32 HttpCacheEntry::Age() const
{
    auto residentTime = FDateTime::UtcNow() - responseTime;
    return correctedInitialAge + residentTime.GetTotalSeconds();
}


bool HttpCacheEntry::IsFresh() const
{
    if (Age() > maxAge)
    {
        return false;
    }
    
    return true;
}


bool HttpCacheEntry::Serialize(SerializationContext& context)
{
    return SERIALIZE_PROPERTY(context, payload)
        && SERIALIZE_PROPERTY(context, date)
        && SERIALIZE_PROPERTY(context, requestTime)
        && SERIALIZE_PROPERTY(context, responseTime)
        && SERIALIZE_PROPERTY(context, correctedInitialAge)
        && SERIALIZE_PROPERTY(context, maxAge)
        && SERIALIZE_PROPERTY(context, headers)
        && SERIALIZE_PROPERTY(context, contentType)
        && SERIALIZE_PROPERTY(context, responseCode)
        && SERIALIZE_PROPERTY(context, url)
        && SERIALIZE_PROPERTY(context, urlHash)
        && SERIALIZE_PROPERTY(context, contentHash)
        && SERIALIZE_PROPERTY(context, onDisk)
        && SERIALIZE_PROPERTY(context, valid);
}
