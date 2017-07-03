// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "DriftSchemas.h"
#include "JsonRequestManager.h"
#include "DriftAPI.h"
#include "DriftEvent.h"

#include "Tickable.h"


DECLARE_LOG_CATEGORY_EXTERN(LogDriftMessages, Log, All);


struct FMessageQueueEntry;


DECLARE_MULTICAST_DELEGATE_OneParam(FDriftMessageQueueDelegate, const FMessageQueueEntry&);


class FDriftMessageQueue : public FTickableGameObject
{
public:
    FDriftMessageQueue();
    ~FDriftMessageQueue();

    /**
     * FTickableGameObject overrides
     */
    void Tick(float DeltaTime) override;
    bool IsTickable() const override { return true; }

    TStatId GetStatId() const override;

    /**
     * API
     */
    void SetRequestManager(TSharedPtr<JsonRequestManager> newRequestManager);
    void SetMessageQueueUrl(const FString& newMessageQueueUrl);

    void SendMessage(const FString& urlTemplate, const FString& queue, JsonValue&& message);
    void SendMessage(const FString& urlTemplate, const FString& queue, JsonValue&& message, int timeoutSeconds);

    FDriftMessageQueueDelegate& OnMessageQueueMessage(const FString& queue);

private:
    void GetMessages();
    void ProcessMessage(const FString& queue, const FMessageQueueEntry& message);

private:
    TWeakPtr<JsonRequestManager> requestManager;
    FString messageQueueUrl;

    TMap<FString, FDriftMessageQueueDelegate> messageHandlers;

    TWeakPtr<HttpRequest> currentPoll;
    
    int32 lastMessageNumber = 0;
    
    float fetchDelay = 0.0f;
};


struct FMessageQueueEntry
{
    int32 exchange_id;
    // The sending player
    int32 sender_id;
    // Always incrementing message ID
    int32 message_number;
    
    FString message_id;
    FString exchange;
    // The queue name
    FString queue;
    
    // Time when message was sent
    FDateTime timestamp;
    // Time when message expires
    FDateTime expires;
    
    // The actual message content
    JsonValue payload{ rapidjson::kObjectType };
    
    FMessageQueueEntry()
    {
    }
    
    FMessageQueueEntry(const FMessageQueueEntry& other)
    : exchange_id(other.exchange_id)
    , sender_id(other.sender_id)
    , message_number(other.message_number)
    , message_id(other.message_id)
    , exchange(other.exchange)
    , queue(other.queue)
    , timestamp(other.timestamp)
    , expires(other.expires)
    {
        payload.CopyFrom(other.payload, JsonArchive::Allocator());
    }
    
    FMessageQueueEntry(FMessageQueueEntry&& other)
    : exchange_id(MoveTemp(other.exchange_id))
    , sender_id(MoveTemp(other.sender_id))
    , message_number(MoveTemp(other.message_number))
    , message_id(MoveTemp(other.message_id))
    , exchange(MoveTemp(other.exchange))
    , queue(MoveTemp(other.queue))
    , timestamp(MoveTemp(other.timestamp))
    , expires(MoveTemp(other.expires))
    {
        payload = MoveTemp(other.payload);
    }
    
    bool Serialize(SerializationContext& context);
};
