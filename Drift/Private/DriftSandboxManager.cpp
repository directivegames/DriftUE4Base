// Copyright 2022 Directive Games Limited - All Rights Reserved

#include "DriftSandboxManager.h"

DEFINE_LOG_CATEGORY(LogDriftSandbox);
static const FString SandboxMessageQueue(TEXT("sandbox"));

FDriftSandboxManager::FDriftSandboxManager(TSharedPtr<IDriftMessageQueue> InMessageQueue)
    : MessageQueue{MoveTemp(InMessageQueue)}
{
    MessageQueue->OnMessageQueueMessage(SandboxMessageQueue).AddRaw(this, &FDriftSandboxManager::HandleSandboxEvent);
}

FDriftSandboxManager::~FDriftSandboxManager()
{
    MessageQueue->OnMessageQueueMessage(SandboxMessageQueue).RemoveAll(this);
}

void FDriftSandboxManager::SetRequestManager(TSharedPtr<JsonRequestManager> RootRequestManager)
{
    RequestManager = MoveTemp(RootRequestManager);
}

void FDriftSandboxManager::ConfigureSession(const FDriftEndpointsResponse& DriftEndpoints, int32 InPlayerId)
{
    PlayerId = InPlayerId;
    SandboxURL = DriftEndpoints.sandbox;
}

bool FDriftSandboxManager::JoinSandbox(const int32 SandboxId, FString Queue, FJoinSandboxFinishedDelegate Delegate)
{
    if (PlayerId == INDEX_NONE)
	{
		UE_LOG(LogDriftSandbox, Error, TEXT("Trying to join a sandbox without a PlayerId"));
		(void)Delegate.ExecuteIfBound(false, "No PlayerId");
		return false;
	}

	UE_LOG(LogDriftSandbox, Log, TEXT("Joining sandbox with id: '%d'"), SandboxId);

    FString URL = SandboxURL;
    if( !URL.EndsWith("/"))
        URL += "/";
    URL +=  FString::FromInt(SandboxId);

	JsonValue Payload{rapidjson::kObjectType};
    Payload.SetField("queue", Queue);
    const auto Request = RequestManager->Put(URL, Payload, HttpStatusCodes::Created);
	Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		UE_LOG(LogDriftSandbox, Log, TEXT("Join Sandbox success response :\n%s'"), *Doc.ToString());

		(void)Delegate.ExecuteIfBound(true, "");
	});
	Request->OnError.BindLambda([Delegate](ResponseContext& Context)
	{
	    UE_LOG(LogDriftSandbox, Log, TEXT("Join Sandbox failed :'%s'"), *Context.error);
		(void)Delegate.ExecuteIfBound(false, Context.error);
	});

	return Request->Dispatch();
}

void FDriftSandboxManager::HandleSandboxEvent(const FMessageQueueEntry& Message)
{
    if (Message.sender_id != FDriftMessageQueue::SenderSystemID && Message.sender_id != PlayerId)
    {
        UE_LOG(LogDriftSandbox, Error, TEXT("HandleSandboxEvent - Ignoring message from sender '%d'"), Message.sender_id);
        return;
    }

    if (!Message.payload.HasField("event") || !Message.payload.HasField("data"))
    {
        UE_LOG(LogDriftSandbox, Error, TEXT("HandleSandboxEvent - No event or data field in message. Discarding the event."));
        return;
    }

    const auto Event = Message.payload.FindField("event").GetString();
    const auto EventData = Message.payload.FindField("data");

    UE_LOG(LogDriftSandbox, Verbose, TEXT("HandleSandboxEvent - Incoming event '%s'"), *Event);

    if (Event.Equals("PlayerSessionReserved"))
    {
        if (!EventData.HasField("connection_info"))
        {
            UE_LOG(LogDriftSandbox, Error, TEXT("HandleSandboxEvent - Event data doesn't contain 'connection_string'. Discarding the event."));
            return;
        }
        const auto MatchConnectionString = EventData.FindField("connection_info").GetString();
        OnSandboxJoinStatusChangedDelegate.Broadcast(MatchConnectionString, true);
        return;
    }

    if (Event.Equals("SessionCreationFailed"))
    {
        const auto JoinError = EventData.FindField("error").GetString();
        OnSandboxJoinStatusChangedDelegate.Broadcast(JoinError, false);
        return;
    }
}
