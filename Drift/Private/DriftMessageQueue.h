// Copyright 2016-2017 Directive Games Limited - All Rights Reserved

#pragma once

#include "DriftSchemas.h"
#include "JsonRequestManager.h"
#include "DriftAPI.h"
#include "DriftEvent.h"
#include "IDriftMessageQueue.h"

#include "Tickable.h"


DECLARE_LOG_CATEGORY_EXTERN(LogDriftMessages, Log, All);


class FDriftMessageQueue : public FTickableGameObject, public IDriftMessageQueue
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

    void SendMessage(const FString& urlTemplate, const FString& queue, JsonValue&& message) override;
    void SendMessage(const FString& urlTemplate, const FString& queue, JsonValue&& message, int timeoutSeconds) override;

    virtual FDriftMessageQueueDelegate& OnMessageQueueMessage(const FString& queue) override;

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


