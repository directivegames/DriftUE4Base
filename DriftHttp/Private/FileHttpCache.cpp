// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#include "DriftHttpPCH.h"

#include "FileHttpCache.h"
#include "HttpRequest.h"
#include "FileHttpCacheFactory.h"
#include "CachedHttpResponse.h"
#include "JsonArchive.h"
#include "Details/DateHelper.h"

#include "SecureHash.h"

#if PLATFORM_PS4
#include "PS4File.h"
#endif

DEFINE_LOG_CATEGORY(LogHttpCache);


const int32 MAX_INLINE_CACHED_CONTENT_SIZE = 1024;
const int32 HTTP_CACHE_INDEX_VERSION = 1;


FString GetCachePath()
{
#if PLATFORM_PS4
    return FPS4PlatformFile::GetTempDirectory();
#else
    #ifdef WITH_FPATHS_PROJECTDIR
        return FPaths::ProjectSavedDir();
    #else
        return FPaths::GameSavedDir();
    #endif
#endif
}


FileHttpCache::FileHttpCache()
: cacheDir{ FPaths::Combine(*GetCachePath(), TEXT("HttpCache")) }
, cacheVersion{ HTTP_CACHE_INDEX_VERSION }
{
    UE_LOG(LogHttpCache, Log, TEXT("Initializing FileHttpCache at: %s"), *cacheDir);

    LoadCache();
}


void FileHttpCache::CacheResponse(const ResponseContext& context)
{
    auto cacheHeader = context.response->GetHeader(TEXT("Cache-Control"));
    if (!(cacheHeader.Contains(TEXT("no-cache")) || cacheHeader.Contains(TEXT("no-store"))))
    {
        int32 maxAge = 0;
        if (FParse::Value(*cacheHeader, TEXT("max-age="), maxAge))
        {
            if (maxAge > 0)
            {
                auto dateHeader = context.response->GetHeader(TEXT("Date"));
                FDateTime dateValue;
                if (!internal::ParseRfc7231DateTime(*dateHeader, dateValue))
                {
                    UE_LOG(LogHttpCache, Verbose, TEXT("Failed to parse 'Date:' header, not safe to cache"));
                    return;
                }

                auto url = context.request->GetURL();
                auto& entry = data.Emplace(url);
                entry.url = url;
                entry.headers = context.response->GetAllHeaders();
                entry.maxAge = maxAge;
                entry.date = dateValue;
                entry.requestTime = context.sent;
                entry.responseTime = context.received;
                entry.responseCode = context.responseCode;
                entry.contentType = context.response->GetContentType();

                entry.correctedInitialAge = CalculateCorrectedInitialAge(context.response, entry).GetTotalSeconds();
                
                FString urlHash;
                auto path = index.Find(url);
                if (path)
                {
                    urlHash = *path;
                }
                else
                {
                    urlHash = FMD5::HashAnsiString(*url);
                }

                entry.urlHash = urlHash;
                entry.contentHash = GetContentHash(context.response);
                
                if (context.response->GetContentLength() < MAX_INLINE_CACHED_CONTENT_SIZE)
                {
                    entry.payload = context.response->GetContent();
                }
                else
                {
                    entry.onDisk = true;
                    if (!SaveBody(urlHash, context.response->GetContent()))
                    {
                        UE_LOG(LogHttpCache, Error, TEXT("Failed to save cache body as '%s'"), *urlHash);
                        return;
                    }
                }
                
                if (!SaveResponse(urlHash, entry))
                {
                    UE_LOG(LogHttpCache, Error, TEXT("Failed to save cache entry as '%s.meta'"), *urlHash);
                    return;
                }

                index.Add(url, urlHash);
                
                SaveIndex();
            }
        }
    }
}


FHttpResponsePtr FileHttpCache::GetCachedResponse(const FString& url)
{
    // TODO: Consider invalidation by the Vary: header
    auto response = data.Find(url);
    if (response && response->valid && response->IsFresh())
    {
        return MakeResponse(*response);
    }
    
    auto responseFile = index.Find(url);
    if (responseFile)
    {
        auto& entry = data.Emplace(url);
        if (LoadResponse(*responseFile, entry))
        {
            if (entry.valid && entry.IsFresh())
            {
                return MakeResponse(entry);
            }
        }
    }
    return nullptr;
}


void FileHttpCache::LoadCache()
{
    auto indexPath = FPaths::Combine(*cacheDir, TEXT("index.json"));
    
    FString fileContent;
    if (!FFileHelper::LoadFileToString(fileContent, *indexPath))
    {
        return;
    }
    
    JsonValue indexObject;
    if (!JsonArchive::LoadObject(*fileContent, indexObject))
    {
        UE_LOG(LogHttpCache, Error, TEXT("Failed to parse cache index file"));
        return;
    }

    if (!indexObject.HasMember(TEXT("entries")) || !indexObject.HasMember(TEXT("version")))
    {
        UE_LOG(LogHttpCache, Error, TEXT("Cache index file missing expected content"));
        return;
    }

    auto& versionValue = indexObject[TEXT("version")];
    if (!versionValue.IsInt())
    {
        UE_LOG(LogHttpCache, Error, TEXT("Cache index version is invalid"));
        return;
    }
    
    int32 version = versionValue.GetInt();
    if (version > cacheVersion)
    {
        UE_LOG(LogHttpCache, Error, TEXT("Cache index version is too high"));
        return;
    }

    auto& entries = indexObject[TEXT("entries")];
    if (!entries.IsObject())
    {
        UE_LOG(LogHttpCache, Error, TEXT("Cache index entries are invalid"));
        return;
    }

    for (auto& member : entries.GetObject())
    {
        FString key;
        FString value;
        if (JsonArchive::LoadObject(member.name, key) && JsonArchive::LoadObject(member.value, value))
        {
            index.Add(key, value);
        }
    }

    UE_LOG(LogHttpCache, Verbose, TEXT("Loaded %d cache entires"), entries.MemberCount());
}


void FileHttpCache::SaveIndex()
{
    JsonValue indexObject{ rapidjson::kObjectType };
    JsonArchive::AddMember(indexObject, TEXT("version"), cacheVersion);
    JsonValue indexEntries{ rapidjson::kObjectType };
    for (const auto& entry : index)
    {
        JsonArchive::AddMember(indexEntries, *entry.Key, *entry.Value);
    }
    JsonArchive::AddMember(indexObject, TEXT("entries"), indexEntries);
    
    auto indexContent = JsonArchive::ToString(indexObject);
    auto indexPath = FPaths::Combine(*cacheDir, TEXT("index.json"));
    
    if (!FFileHelper::SaveStringToFile(indexContent, *indexPath))
    {
        UE_LOG(LogHttpCache, Error, TEXT("Failed to save cache index file: %s"), *indexPath);
    }
    else
    {
        UE_LOG(LogHttpCache, Verbose, TEXT("Cache index file saved: %s"), *indexPath);
    }
}


FString FileHttpCache::MakeEntryPath(const FString &name) const
{
    return FPaths::Combine(*cacheDir, *name.Left(2), *name.Mid(2, 2), *name);
}


bool FileHttpCache::LoadResponse(const FString &name, HttpCacheEntry& entry)
{
    auto fullPath = MakeEntryPath(name) + TEXT(".meta");

    FString fileContent;
    if (!FFileHelper::LoadFileToString(fileContent, *fullPath))
    {
        UE_LOG(LogHttpCache, Error, TEXT("Failed to load cache file: %s"), *fullPath);
        return false;
    }
    
    if (!JsonArchive::LoadObject(*fileContent, entry))
    {
        UE_LOG(LogHttpCache, Error, TEXT("Failed to parse cache entry file"));
        return false;
    }

    return true;
}


bool FileHttpCache::SaveResponse(const FString& name, const HttpCacheEntry& entry)
{
    FString fileContent;
    if (!JsonArchive::SaveObject(entry, fileContent))
    {
        UE_LOG(LogHttpCache, Error, TEXT("Failed to serialize cache entry"));
        return false;
    }

    auto fullPath = MakeEntryPath(name) + TEXT(".meta");
    return FFileHelper::SaveStringToFile(fileContent, *fullPath);
}


bool FileHttpCache::LoadBody(const FString& name, TArray<uint8>& body)
{
    return FFileHelper::LoadFileToArray(body, *MakeEntryPath(name));
}


bool FileHttpCache::SaveBody(const FString& name, const TArray<uint8>& body)
{
    return FFileHelper::SaveArrayToFile(body, *MakeEntryPath(name));
}


FString FileHttpCache::GetContentHash(const FHttpResponsePtr& response) const
{
    FSHAHash hash;
    FSHA1 hashState;
    
    hashState.Update(&response->GetContent()[0], response->GetContent().Num());
    hashState.Final();
    hashState.GetHash(&hash.Hash[0]);
    
    return hash.ToString();
}


FTimespan FileHttpCache::CalculateCorrectedInitialAge(const FHttpResponsePtr &response, HttpCacheEntry &entry) const
{
    /**
     * Implementation follows the pattern laid out in the HTTP/1.1 RFC7234
     * https://tools.ietf.org/html/rfc7234
     */
    int32 age = 0;
    
    auto ageHeader = response->GetHeader(TEXT("Age"));
    if (!ageHeader.IsEmpty())
    {
        const TCHAR* pointer = *ageHeader;
        FString ignore;
        if (FParse::Token(pointer, ignore, false))
        {
            FCString::Atoi(pointer);
        }
    }

    auto ageValue = FTimespan::FromSeconds(age);
    auto apparentAge = FMath::Max(FTimespan::FromSeconds(0.0), entry.responseTime - entry.date);
    auto responseDelay = entry.responseTime - entry.requestTime;
    auto correctedAgeValue = ageValue + responseDelay;

    return FMath::Max(apparentAge, correctedAgeValue);
}


FHttpResponsePtr FileHttpCache::MakeResponse(HttpCacheEntry& entry)
{
    TSharedPtr<CachedHttpResponse, ESPMode::ThreadSafe> response = MakeShareable(new CachedHttpResponse());

    response->url = entry.url;
    response->contentType = entry.contentType;
    response->responseCode = entry.responseCode;

    if (!entry.onDisk)
    {
        response->payload = entry.payload;
    }
    else
    {
        LoadBody(entry.urlHash, response->payload);
    }

    auto hash = GetContentHash(response);
    if (entry.contentHash != hash)
    {
        UE_LOG(LogHttpCache, Error, TEXT("Cached response checksum mismatch"));
        entry.valid = false;
        return nullptr;
    }

    FString name;
    FString value;
    for (const auto& header : entry.headers)
    {
        if (header.Split(TEXT(": "), &name, &value))
        {
            response->headers.Add(name, value);
        }
    }
    response->headers.Add(TEXT("Age"), FString::Printf(TEXT("%d"), entry.Age()));

    return response;
}


TSharedPtr<IHttpCache> FileHttpCacheFactory::Create()
{
    return MakeShareable(new FileHttpCache());
}
