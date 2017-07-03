// Copyright 2015-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "RequestManager.h"
#include "JsonArchive.h"


/**
* Special request provider that deals with json based payload and response
* For PUT and POST methods, non FString based payload can be used
* Currently the only supported payload classes are UStructs
* */
class DRIFTHTTP_API JsonRequestManager : public RequestManager
{
public:
    template <typename TPayload>
    TSharedRef<HttpRequest> Patch(const FString& url, const TPayload& payload);
    template <typename TPayload>
    TSharedRef<HttpRequest> Patch(const FString& url, const TPayload& payload, HttpStatusCodes expectedCode);
    template <typename TPayload>
    TSharedRef<HttpRequest> Put(const FString& url, const TPayload& payload);
    template <typename TPayload>
    TSharedRef<HttpRequest> Put(const FString& url, const TPayload& payload, HttpStatusCodes expectedCode);
    template <typename TPayload>
    TSharedRef<HttpRequest> Post(const FString& url, const TPayload& payload);
    template <typename TPayload>
    TSharedRef<HttpRequest> Post(const FString& url, const TPayload& payload, HttpStatusCodes expectedCode);

    using RequestManager::CreateRequest;

    template<class TPayload>
    TSharedRef<HttpRequest> CreateRequest(HttpMethods method, const FString& url, const TPayload& payload)
    {
        return CreateRequest(method, url, payload, HttpStatusCodes::Undefined);
    }
    
    template<class TPayload>
    TSharedRef<HttpRequest> CreateRequest(HttpMethods method, const FString& url, const TPayload& payload, HttpStatusCodes expectedCode)
    {
        FString payloadString;
        JsonArchive::SaveObject(payload, payloadString);
        return RequestManager::CreateRequest(method, url, FString(payloadString));
    }

    void SetApiKey(const FString& apiKey);

protected:
    virtual void AddCustomHeaders(TSharedRef<HttpRequest> request) const override;
    
private:
    FString apiKey_;
};


template<>
TSharedRef<HttpRequest> JsonRequestManager::CreateRequest<FString>(HttpMethods method, const FString& url, const FString& payload);


template<>
TSharedRef<HttpRequest> JsonRequestManager::CreateRequest<FString>(HttpMethods method, const FString& url, const FString& payload, HttpStatusCodes expectedCode);


template <typename TPayload>
TSharedRef<HttpRequest> JsonRequestManager::Patch(const FString& url, const TPayload& payload)
{
    return Patch(url, payload, HttpStatusCodes::Ok);
}


template <typename TPayload>
TSharedRef<HttpRequest> JsonRequestManager::Patch(const FString& url, const TPayload& payload, HttpStatusCodes expectedCode)
{
    return CreateRequest(HttpMethods::XPATCH, url, payload, expectedCode);
}


template <typename TPayload>
TSharedRef<HttpRequest> JsonRequestManager::Post(const FString& url, const TPayload& payload)
{
    return Post(url, payload, HttpStatusCodes::Created);
}


template <typename TPayload>
TSharedRef<HttpRequest> JsonRequestManager::Post(const FString& url, const TPayload& payload, HttpStatusCodes expectedCode)
{
    return CreateRequest(HttpMethods::XPOST, url, payload, expectedCode);
}


template <typename TPayload>
TSharedRef<HttpRequest> JsonRequestManager::Put(const FString& url, const TPayload& payload)
{
    return Put(url, payload, HttpStatusCodes::Ok);
}


template <typename TPayload>
TSharedRef<HttpRequest> JsonRequestManager::Put(const FString& url, const TPayload& payload, HttpStatusCodes expectedCode)
{
    return CreateRequest(HttpMethods::XPUT, url, payload, expectedCode);
}
