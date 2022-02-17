// Copyright 2022 Directive Games Limited - All Rights Reserved

#include "DriftMatchPlacementManager.h"


DEFINE_LOG_CATEGORY(LogDriftMatchPlacement);

static const FString MatchPlacementMessageQueue(TEXT("match_placements"));

FDriftMatchPlacementManager::FDriftMatchPlacementManager(TSharedPtr<IDriftMessageQueue> InMessageQueue)
	: MessageQueue{MoveTemp(InMessageQueue)}
{
	MessageQueue->OnMessageQueueMessage(MatchPlacementMessageQueue).AddRaw(this, &FDriftMatchPlacementManager::HandleMatchPlacementEvent);

	ResetCurrenMatchPlacement();
}

FDriftMatchPlacementManager::~FDriftMatchPlacementManager()
{
	MessageQueue->OnMessageQueueMessage(MatchPlacementMessageQueue).RemoveAll(this);
}

void FDriftMatchPlacementManager::SetRequestManager(TSharedPtr<JsonRequestManager> RootRequestManager)
{
	RequestManager = MoveTemp(RootRequestManager);
}

void FDriftMatchPlacementManager::ConfigureSession(const FDriftEndpointsResponse& DriftEndpoints, int32 InPlayerId)
{
	PlayerId = InPlayerId;

	MatchPlacementsURL = DriftEndpoints.match_placements;

	if (HasSession())
	{
		InitializeLocalState();
	}
}

void FDriftMatchPlacementManager::InitializeLocalState()
{
	UE_LOG(LogDriftMatchPlacement, Log, TEXT("Querying for initial match placement state"));

	const auto Request = RequestManager->Get(MatchPlacementsURL);
	Request->OnResponse.BindLambda([this](ResponseContext& Context, JsonDocument& Doc)
	{
		UE_LOG(LogDriftMatchPlacement, Verbose, TEXT("InitializeLocalState response:'n'%s'"), *Doc.ToString());

		const auto Response = Doc.GetObject();
		if (Response.Num() == 0)
		{
			UE_LOG(LogDriftMatchPlacement, Warning, TEXT("No match placement found when querying for initial state. Should return 404, not '%d'"), Context.response->GetResponseCode());
			ResetCurrenMatchPlacement();
			return;
		}

		FDriftMatchPlacementResponse MatchPlacementResponse{};
		if (!MatchPlacementResponse.FromJson(Doc.GetInternalValue()->AsObject()))
		{
			UE_LOG(LogDriftMatchPlacement, Error, TEXT("Failed to serialize get match placement response"));
			return;
		}

		CacheMatchPlacement(MatchPlacementResponse);
	});
	Request->OnError.BindLambda([this](ResponseContext& Context)
	{
		if (Context.response->GetResponseCode() == EHttpResponseCodes::NotFound)
		{
			UE_LOG(LogDriftMatchPlacement, Log, TEXT("No existing match placement found"));
			Context.errorHandled = true;
		}
		else
		{
			FString Error;
			Context.errorHandled = GetResponseError(Context, Error);
			UE_LOG(LogDriftMatchPlacement, Error, TEXT("InitializeLocalState - Error fetching existing match placement, Response code %d, error: '%s'"), Context.responseCode, *Error);
		}

		ResetCurrenMatchPlacement();
	});

	Request->Dispatch();
}

bool FDriftMatchPlacementManager::QueryMatchPlacement(FQueryMatchPlacementCompletedDelegate Delegate)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftMatchPlacement, Error, TEXT("Trying to query match placement without a session"));
		(void)Delegate.ExecuteIfBound(false, "", "No backend connection");
		return false;
	}

	UE_LOG(LogDriftMatchPlacement, Log, TEXT("Querying for current match placement"));

	const auto Request = RequestManager->Get(MatchPlacementsURL);
	Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		UE_LOG(LogDriftMatchPlacement, Verbose, TEXT("QueryMatchPlacement response:'n'%s'"), *Doc.ToString());

		const auto Response = Doc.GetObject();
		if (Response.Num() == 0)
		{
			UE_LOG(LogDriftMatchPlacement, Log, TEXT("No match placement found"));

			ResetCurrenMatchPlacement();

			(void)Delegate.ExecuteIfBound(true, "", "");
			return;
		}

		FDriftMatchPlacementResponse MatchPlacementResponse{};
		if (!MatchPlacementResponse.FromJson(Doc.GetInternalValue()->AsObject()))
		{
			UE_LOG(LogDriftMatchPlacement, Error, TEXT("Failed to serialize get match placement response"));
			return;
		}

		ResetCurrenMatchPlacement();

	    CacheMatchPlacement(MatchPlacementResponse);

	    (void)Delegate.ExecuteIfBound(true, CurrentMatchPlacementId, "");
	});
	Request->OnError.BindLambda([Delegate](ResponseContext& Context)
	{
		FString Error;
		Context.errorHandled = GetResponseError(Context, Error);
		(void)Delegate.ExecuteIfBound(false, "", Error);
	});

	return Request->Dispatch();
}

bool FDriftMatchPlacementManager::CreateMatchPlacement(FDriftMatchPlacementProperties MatchPlacementProperties, FCreateMatchPlacementCompletedDelegate Delegate)
{
	if (!HasSession())
	{
		UE_LOG(LogDriftMatchPlacement, Error, TEXT("Trying to create a match placement without a session"));
		(void)Delegate.ExecuteIfBound(false, "", "No backend connection");
		return false;
	}

	UE_LOG(LogDriftMatchPlacement, Log, TEXT("Creating match placement with properties: '%s'"), *MatchPlacementProperties.ToString());

	JsonValue Payload{rapidjson::kObjectType};
    JsonArchive::AddMember(Payload, TEXT("queue"), *MatchPlacementProperties.QueueName);
    JsonArchive::AddMember(Payload, TEXT("map_name"), *MatchPlacementProperties.MapName);

	if (MatchPlacementProperties.Identifier.IsSet())
	{
		JsonArchive::AddMember(Payload, TEXT("identifier"), *MatchPlacementProperties.Identifier.GetValue());
	}

	if (MatchPlacementProperties.MaxPlayers.IsSet())
	{
		JsonArchive::AddMember(Payload, TEXT("max_players"), MatchPlacementProperties.MaxPlayers.GetValue());
	}

	if (MatchPlacementProperties.CustomData.IsSet())
	{
		JsonArchive::AddMember(Payload, TEXT("custom_data"), MatchPlacementProperties.CustomData.GetValue());
	}

	const auto Request = RequestManager->Post(MatchPlacementsURL, Payload);
	Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
	{
		UE_LOG(LogDriftMatchPlacement, Log, TEXT("Match placement created"));

		UE_LOG(LogDriftMatchPlacement, Verbose, TEXT("CreateLobby response:'n'%s'"), *Doc.ToString());

		FDriftMatchPlacementResponse MatchPlacementResponse{};
		if (!MatchPlacementResponse.FromJson(Doc.GetInternalValue()->AsObject()))
		{
			UE_LOG(LogDriftMatchPlacement, Error, TEXT("Failed to serialize create match placement response"));
			return;
		}

		ResetCurrenMatchPlacement();
		CacheMatchPlacement(MatchPlacementResponse);
		(void)Delegate.ExecuteIfBound(true, CurrentMatchPlacementId, "");
	});
	Request->OnError.BindLambda([Delegate](ResponseContext& Context)
	{
		FString Error;
		Context.errorHandled = GetResponseError(Context, Error);
		(void)Delegate.ExecuteIfBound(false, "", Error);
	});

	return Request->Dispatch();
}

void FDriftMatchPlacementManager::HandleMatchPlacementEvent(const FMessageQueueEntry& Message)
{
	if (Message.sender_id != FDriftMessageQueue::SenderSystemID && Message.sender_id != PlayerId)
	{
		UE_LOG(LogDriftMatchPlacement, Error, TEXT("HandleMatchPlacementEvent - Ignoring message from sender '%d'"), Message.sender_id);
		return;
	}

	const auto Event = Message.payload.FindField("event").GetString();
	const auto EventData = Message.payload.FindField("data");

	UE_LOG(LogDriftMatchPlacement, Verbose, TEXT("HandleMatchPlacementEvent - Incoming event '%s'"), *Event);

	if (!EventData.HasField("placement_id"))
	{
		UE_LOG(LogDriftMatchPlacement, Error, TEXT("HandleMatchPlacementEvent - Event data doesn't contain 'placement_id'. Discarding the event. Current cached placement id: '%s'. Querying for the current match placement to sync up just in case."), *CurrentMatchPlacementId);
		QueryMatchPlacement({});
		return;
	}

	const auto MatchPlacementId = EventData.FindField("placement_id").GetString();

	// Verify that this event is relevant to us
	if (MatchPlacementId != CurrentMatchPlacementId)
	{
		UE_LOG(LogDriftMatchPlacement, Warning, TEXT("HandleMatchPlacementEvent - Cached match placement '%s' does not match the event match placement '%s'"), *CurrentMatchPlacementId, *MatchPlacementId);
        return;
	}

    CurrentMatchPlacement->MatchPlacementStatus = ParseEvent(Event);

	switch (CurrentMatchPlacement->MatchPlacementStatus)
	{
		case EDriftMatchPlacementStatus::Issued:
		{
			if (!EventData.HasField("connection_string"))
			{
				UE_LOG(LogDriftMatchPlacement, Error, TEXT("HandleMatchPlacementEvent - Issued - Event data missing 'connection_string' field. Syncing up the match placement state just in case."));
				QueryMatchPlacement({});
				return;
			}

			if (!EventData.HasField("connection_options"))
			{
				UE_LOG(LogDriftMatchPlacement, Error, TEXT("HandleMatchPlacementEvent - Issued - Event data missing 'connection_options' field. Syncing up the match placement state just in case."));
				QueryMatchPlacement({});
				return;
			}

			CurrentMatchPlacement->ConnectionString = EventData.FindField("connection_string").GetString();
			CurrentMatchPlacement->ConnectionOptions = EventData.FindField("connection_options").GetString();

			OnMatchPlacementStatusChangedDelegate.Broadcast(CurrentMatchPlacementId, CurrentMatchPlacement->MatchPlacementStatus);
			break;
		}

		case EDriftMatchPlacementStatus::Cancelled:
		case EDriftMatchPlacementStatus::TimedOut:
		case EDriftMatchPlacementStatus::Failed:
		{
		    OnMatchPlacementStatusChangedDelegate.Broadcast(CurrentMatchPlacementId, CurrentMatchPlacement->MatchPlacementStatus);
			break;
		}

		case EDriftMatchPlacementStatus::Unknown:
		default:
			UE_LOG(LogDriftMatchPlacement, Error, TEXT("HandleMatchPlacementEvent - Unknown event '%s'. Syncing up the lobby state just in case."), *Event);
			QueryMatchPlacement({});
	}
}

EDriftMatchPlacementStatus FDriftMatchPlacementManager::ParseEvent(const FString& EventName)
{
	if (EventName == TEXT("MatchPlacementIssued")) { return EDriftMatchPlacementStatus::Issued; }
	if (EventName == TEXT("MatchPlacementFulfilled")) { return EDriftMatchPlacementStatus::Fulfilled; }
	if (EventName == TEXT("MatchPlacementCancelled")) { return EDriftMatchPlacementStatus::Cancelled; }
	if (EventName == TEXT("MatchPlacementTimedOut")) { return EDriftMatchPlacementStatus::TimedOut; }
	if (EventName == TEXT("MatchPlacementFailed")) { return EDriftMatchPlacementStatus::Failed; }

	return EDriftMatchPlacementStatus::Unknown;
}

bool FDriftMatchPlacementManager::HasSession() const
{
	return !MatchPlacementsURL.IsEmpty() && RequestManager.IsValid();
}

void FDriftMatchPlacementManager::CacheMatchPlacement(const FDriftMatchPlacementResponse& MatchPlacementResponse)
{
	CurrentMatchPlacementId = MatchPlacementResponse.PlacementId;
    CurrentMatchPlacementURL = MatchPlacementResponse.MatchPlacementURL;

	CurrentMatchPlacement = MakeShared<FDriftMatchPlacement>(
		CurrentMatchPlacementId,
		MatchPlacementResponse.MapName,
		MatchPlacementResponse.PlayerId,
		MatchPlacementResponse.MaxPlayers,
		ParseEvent(MatchPlacementResponse.Status),
		MatchPlacementResponse.CustomData,
		CurrentMatchPlacementURL
	);

	if (!MatchPlacementResponse.ConnectionString.IsEmpty())
	{
		CurrentMatchPlacement->ConnectionString = MatchPlacementResponse.ConnectionString;
		CurrentMatchPlacement->ConnectionOptions = MatchPlacementResponse.ConnectionOptions.IsEmpty() ? "SpectatorOnly=1" : MatchPlacementResponse.ConnectionOptions;
	}

	UE_LOG(LogDriftMatchPlacement, Log, TEXT("Mtch placement cached: '%s'"), *CurrentMatchPlacementId);
	OnMatchPlacementStatusChangedDelegate.Broadcast(CurrentMatchPlacementId, CurrentMatchPlacement->MatchPlacementStatus);
}

void FDriftMatchPlacementManager::ResetCurrenMatchPlacement()
{
	CurrentMatchPlacement.Reset();
	CurrentMatchPlacementId.Empty();
	CurrentMatchPlacementURL.Empty();

	UE_LOG(LogDriftMatchPlacement, Verbose, TEXT("Current match placement state reset"));
}

bool FDriftMatchPlacementManager::GetResponseError(const ResponseContext& Context, FString& Error)
{
	Error = "Unknown error";

	if (!Context.response)
	{
		return false;
	}

	JsonDocument Doc;
	Doc.Parse(*Context.response->GetContentAsString());

	if (Doc.HasParseError())
	{
		return false;
	}

	// Check if there is a specific error message provided
	if (Doc.HasField(TEXT("error")))
	{
		const auto ErrorField = Doc[TEXT("error")].GetObject();
		if (const auto ErrorValuePtr = ErrorField.Find("description"))
		{
			Error = ErrorValuePtr->GetString();
			return true;
		}

	}

	// Fallback to the generic error message if provided
	if (Doc.HasField(TEXT("message")))
	{
		Error = Doc[TEXT("message")].GetString();
		return true;
	}

	return false;
}
