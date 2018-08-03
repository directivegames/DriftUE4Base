// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Misc/DateTime.h"


class SerializationContext;


class HttpCacheEntry
{
public:
    TArray<FString> headers;
    TArray<uint8> payload;
    FString contentType;
    int32 responseCode;
    int32 maxAge;
    FDateTime date;
    FDateTime requestTime;
    FDateTime responseTime;
    int32 correctedInitialAge;

    FString url;
    FString urlHash;
    FString contentHash;
    bool onDisk = false;
    bool valid = true;

    int32 Age() const;
    bool IsFresh() const;
    
    bool Serialize(SerializationContext& context);
};
