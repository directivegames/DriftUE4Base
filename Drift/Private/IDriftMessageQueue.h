// Copyright 2020 Directive Games Limited - All Rights Reserved

#pragma once

#include "JsonValueWrapper.h"


struct FMessageQueueEntry;


DECLARE_MULTICAST_DELEGATE_OneParam(FDriftMessageQueueDelegate, const FMessageQueueEntry&);


class IDriftMessageQueue
{
public:
	virtual void SendMessage(const FString& urlTemplate, const FString& queue, JsonValue&& message) = 0;
	virtual void SendMessage(const FString& urlTemplate, const FString& queue, JsonValue&& message, int timeoutSeconds) = 0;

	virtual FDriftMessageQueueDelegate& OnMessageQueueMessage(const FString& queue) = 0;

	virtual ~IDriftMessageQueue() = default;
};
