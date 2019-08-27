// Copyright 2016-2019 Directive Games Limited - All Rights Reserved

#include "DriftMessageQueue.h"

#include "DriftSchemas.h"
#include "Details/UrlHelper.h"
#include "JsonArchive.h"


DEFINE_LOG_CATEGORY(LogDriftMessages);


const float DEFAULT_FETCH_THROTTLE_DELAY_SECONDS = 5.0f;
const float MESSAGE_FETCH_TIMEOUT_SECONDS = 20.0f;


FDriftMessageQueue::FDriftMessageQueue()
{
}


FDriftMessageQueue::~FDriftMessageQueue()
{
    auto pendingPoll = currentPoll.Pin();
    if (pendingPoll.IsValid())
    {
        pendingPoll->Destroy();
    }
}


void FDriftMessageQueue::Tick(float DeltaTime)
{
    if (messageQueueUrl.IsEmpty() || !requestManager.IsValid())
    {
        return;
    }

    if (fetchDelay > 0.0f)
    {
        fetchDelay -= DeltaTime;
        return;
    }

    if (!currentPoll.IsValid())
    {
        GetMessages();
    }
}


TStatId FDriftMessageQueue::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(FDriftMessageQueue, STATGROUP_Tickables);
}


void FDriftMessageQueue::SetRequestManager(TSharedPtr<JsonRequestManager> newRequestManager)
{
    requestManager = newRequestManager;
    fetchDelay = 0.0f;
}


void FDriftMessageQueue::SetMessageQueueUrl(const FString& newMessageQueueUrl)
{
    messageQueueUrl = newMessageQueueUrl;
}


struct FMessageQueueMessage
{
    JsonValue message;
    int expire;
    
    bool Serialize(SerializationContext& context)
    {
        return SERIALIZE_PROPERTY(context, message)
            && SERIALIZE_PROPERTY(context, expire);
    }
};


void FDriftMessageQueue::SendMessage(const FString& urlTemplate, const FString& queue, JsonValue&& message)
{
    SendMessage(urlTemplate, queue, Forward<JsonValue>(message), 0);
}


void FDriftMessageQueue::SendMessage(const FString& urlTemplate, const FString& queue, JsonValue&& message, int timeoutSeconds)
{
    check(!urlTemplate.IsEmpty());
    check(urlTemplate.Contains(TEXT("{queue}")));

    if (queue.IsEmpty())
    {
        UE_LOG(LogDriftMessages, Warning, TEXT("Queue name must not be empty"));

        return;
    }

    auto url = urlTemplate.Replace(TEXT("{queue}"), *queue);
    auto rm = requestManager.Pin();
    if (rm.IsValid())
    {
        FMessageQueueMessage payload{ Forward<JsonValue>(message), timeoutSeconds };
        auto request = rm->Post(url, payload);
        request->OnError.BindLambda([this](ResponseContext& context)
        {
            context.errorHandled = true;
        });
        request->Dispatch();
    }
}


struct FMessageQueue
{
    TMap<FString, TArray<FMessageQueueEntry>> queues;
    
    bool Serialize(SerializationContext& context)
    {
        if (context.IsLoading())
        {
            queues.Empty();
            auto& jValue = context.GetValue();
            if (!jValue.IsObject())
            {
                return false;
            }
            for (auto& member : jValue.GetObject())
            {
                const auto& queue = member.Key;
                auto& entry = queues.Emplace(queue);
                context.SerializeProperty(*queue, entry);
            }
            return true;
        }
        return false;
    }
};


void FDriftMessageQueue::GetMessages()
{
    if (messageQueueUrl.IsEmpty())
    {
        UE_LOG(LogDriftMessages, Error, TEXT("Trying to fetch messages without an endpoint"));
        return;
    }

    if (currentPoll.IsValid())
    {
        UE_LOG(LogDriftMessages, Verbose, TEXT("Already waiting for a response, ignoring request to poll message queue"));
        return;
    }

    auto rm = requestManager.Pin();
    if (rm.IsValid())
    {
        UE_LOG(LogDriftMessages, Verbose, TEXT("Polling message queue..."));

        auto url = messageQueueUrl;
        internal::UrlHelper::AddUrlOption(url, TEXT("messages_after"), lastMessageNumber);
        internal::UrlHelper::AddUrlOption(url, TEXT("timeout"), MESSAGE_FETCH_TIMEOUT_SECONDS);
        auto request = rm->Get(url);
        currentPoll = request;
        request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
        {
            UE_LOG(LogDriftMessages, VeryVerbose, TEXT("Message queue response: %s"), *JsonArchive::ToString(doc));

            FMessageQueue messages;
            if (!JsonArchive::LoadObject(doc, messages))
            {
                context.error = TEXT("Failed to parse message queue response");
                fetchDelay = DEFAULT_FETCH_THROTTLE_DELAY_SECONDS;
                return;
            }
            auto oldLastMessage = lastMessageNumber;
            for (const auto& queue : messages.queues)
            {
                for (const auto& message : queue.Value)
                {
                    ProcessMessage(queue.Key, message);
                    
                    lastMessageNumber = FMath::Max(lastMessageNumber, message.message_number);
                }
            }

            if (oldLastMessage == lastMessageNumber && (FDateTime::UtcNow() - context.sent).GetTotalSeconds() < DEFAULT_FETCH_THROTTLE_DELAY_SECONDS)
            {
                /**
                 * Got an empty reply which was not a timeout; something might be off
                 * make sure we throttle the polling frequency for a while
                 */
                fetchDelay = DEFAULT_FETCH_THROTTLE_DELAY_SECONDS;
            }
            else
            {
                // Getting proper data again, drop throttle
                fetchDelay = 0.0f;
            }

            currentPoll.Reset();
        });
        request->OnError.BindLambda([this](ResponseContext& context)
        {
            currentPoll.Reset();
            fetchDelay = DEFAULT_FETCH_THROTTLE_DELAY_SECONDS;
            context.errorHandled = true;
            return;
        });
        request->Dispatch();
    }
}


void FDriftMessageQueue::ProcessMessage(const FString& queue, const FMessageQueueEntry& message)
{
    UE_LOG(LogDriftMessages, Log, TEXT("Got message: %s\n%s"), *message.message_id, *JsonArchive::ToString(message.payload));

    auto delegate = messageHandlers.Find(queue);
    if (delegate)
    {
        delegate->Broadcast(message);
    }
}


FDriftMessageQueueDelegate& FDriftMessageQueue::OnMessageQueueMessage(const FString& queue)
{
    return messageHandlers.FindOrAdd(queue);
}


bool FMessageQueueEntry::Serialize(SerializationContext& context)
{
    return SERIALIZE_PROPERTY(context, exchange_id)
        && SERIALIZE_PROPERTY(context, sender_id)
        && SERIALIZE_PROPERTY(context, message_number)
        && SERIALIZE_PROPERTY(context, message_id)
        && SERIALIZE_PROPERTY(context, exchange)
        && SERIALIZE_PROPERTY(context, queue)
        && SERIALIZE_PROPERTY(context, timestamp)
        && SERIALIZE_PROPERTY(context, expires)
        && SERIALIZE_PROPERTY(context, payload);
}
