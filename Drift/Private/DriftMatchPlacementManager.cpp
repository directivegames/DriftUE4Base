// Copyright 2022 Directive Games Limited - All Rights Reserved

#include "DriftMatchPlacementManager.h"


DEFINE_LOG_CATEGORY(LogDriftMatchPlacement);

static const FString MatchPlacementMessageQueue(TEXT("match_placements"));

FDriftMatchPlacementManager::FDriftMatchPlacementManager(TSharedPtr<IDriftMessageQueue> InMessageQueue)
	: MessageQueue{MoveTemp(InMessageQueue)}
{
	MessageQueue->OnMessageQueueMessage(MatchPlacementMessageQueue).AddRaw(this, &FDriftMatchPlacementManager::HandleMatchPlacementEvent);

	ResetCurrentMatchPlacement();
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
    PublicPlacementsURL = DriftEndpoints.public_match_placements;

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

	    ResetCurrentMatchPlacement();

		const auto Response = Doc.GetObject();
		if (Response.Num() == 0)
		{
			UE_LOG(LogDriftMatchPlacement, Warning, TEXT("No match placement found when querying for initial state. Should return 404, not '%d'"), Context.response->GetResponseCode());
			return;
		}

		FDriftMatchPlacementResponse MatchPlacementResponse{};
		if (!MatchPlacementResponse.FromJson(Doc.GetInternalValue()->AsObject()))
		{
			UE_LOG(LogDriftMatchPlacement, Error, TEXT("Failed to serialize initial get match placement response"));
			return;
		}

	    // Only react to pending/issued match placements for now
	    const auto Status = ParseStatus(MatchPlacementResponse.Status);
	    if (Status == EDriftMatchPlacementStatus::Issued)
	    {
	        CacheMatchPlacement(MatchPlacementResponse);
	        return;
	    }

	    UE_LOG(LogDriftMatchPlacement, Log, TEXT("Match placement '%s' found, but the status is '%s'. Ignoring."), *MatchPlacementResponse.PlacementId, *MatchPlacementResponse.Status);
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

		ResetCurrentMatchPlacement();
	});

	Request->Dispatch();
}

bool FDriftMatchPlacementManager::GetPlacement(const FString& MatchPlacementId,
    FQueryMatchPlacementCompletedDelegate Delegate)
{
    UE_LOG(LogDriftMatchPlacement, Log, TEXT("Fetching info on match placement '%s'"), *MatchPlacementId);
    FString URL = MatchPlacementsURL;
    if( !URL.EndsWith("/"))
        URL += "/";
    URL +=  MatchPlacementId;
    const auto Request = RequestManager->Get(URL);
    Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
    {
        UE_LOG(LogDriftMatchPlacement, Log, TEXT("Git match placement info"));
        ResetCurrentMatchPlacement();

        const auto Response = Doc.GetObject();
        FDriftMatchPlacementResponse MatchPlacementResponse;
        if (!MatchPlacementResponse.FromJson(Doc.GetInternalValue()->AsObject()))
        {
            UE_LOG(LogDriftMatchPlacement, Error, TEXT("Failed to serialize get match placement response"));
            return;
        }

        // Only react to pending/issued match placements for now
        const auto Status = ParseStatus(MatchPlacementResponse.Status);
        if (Status == EDriftMatchPlacementStatus::Issued || Status == EDriftMatchPlacementStatus::Fulfilled)
        {
            UE_LOG(LogDriftMatchPlacement, Log, TEXT("Caching match placement '%s'"), *MatchPlacementResponse.PlacementId);
            CacheMatchPlacement(MatchPlacementResponse);
            (void)Delegate.ExecuteIfBound(true, CurrentMatchPlacementId, "");
            return;
        }
        UE_LOG(LogDriftMatchPlacement, Log, TEXT("Match placement '%s' found, but the status is '%s'. Ignoring."), *MatchPlacementResponse.PlacementId, *MatchPlacementResponse.Status);

        (void)Delegate.ExecuteIfBound(false, {}, FString::Printf(TEXT("Match placement '%s' not in a usable state"), *MatchPlacementResponse.PlacementId));
    });
    Request->OnError.BindLambda([Delegate](ResponseContext& Context)
    {
        FString Error;
        Context.errorHandled = GetResponseError(Context, Error);
        (void)Delegate.ExecuteIfBound(false, {}, Error);
    });
    return Request->Dispatch();
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

	    ResetCurrentMatchPlacement();

		const auto Response = Doc.GetObject();
		if (Response.Num() == 0)
		{
			UE_LOG(LogDriftMatchPlacement, Log, TEXT("No match placement found"));

			(void)Delegate.ExecuteIfBound(true, "", "");
			return;
		}

		FDriftMatchPlacementResponse MatchPlacementResponse{};
		if (!MatchPlacementResponse.FromJson(Doc.GetInternalValue()->AsObject()))
		{
			UE_LOG(LogDriftMatchPlacement, Error, TEXT("Failed to serialize get match placement response"));
			return;
		}

	    // Only react to pending/issued match placements for now
        const auto Status = ParseStatus(MatchPlacementResponse.Status);
        if (Status == EDriftMatchPlacementStatus::Issued)
        {
            CacheMatchPlacement(MatchPlacementResponse);
            (void)Delegate.ExecuteIfBound(true, CurrentMatchPlacementId, "");
            return;
        }

	    UE_LOG(LogDriftMatchPlacement, Log, TEXT("Match placement '%s' found, but the status is '%s'. Ignoring."), *MatchPlacementResponse.PlacementId, *MatchPlacementResponse.Status);

	    (void)Delegate.ExecuteIfBound(true, "", "");

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

	if (MatchPlacementProperties.IsPublic.IsSet() && MatchPlacementProperties.IsPublic.GetValue())
	{
		JsonArchive::AddMember(Payload, TEXT("is_public"), true);
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

		ResetCurrentMatchPlacement();
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

bool FDriftMatchPlacementManager::JoinMatchPlacement(const FString& MatchPlacementID, FJoinMatchPlacementCompletedDelegate Delegate)
{
    // A bit of a hack to support public match placements
    if (!HasSession())
    {
        UE_LOG(LogDriftMatchPlacement, Error, TEXT("Trying to join a match placement without a session"));
        (void)Delegate.ExecuteIfBound(false, {}, "No backend connection");
        return false;
    }
    if (MatchPlacementID.IsEmpty())
    {
        UE_LOG(LogDriftMatchPlacement, Error, TEXT("PlacementID is empty"));
        (void)Delegate.ExecuteIfBound(false, {}, "Missing PlacementID");
        return false;
    }

    if (CurrentMatchPlacementId.IsEmpty())
        // non-obvious; if you have started your own placement and you're erronously attempting to 'join' it, this will
        // not be empty and we will not attempt to fetch it again. 'Joining' it is basically a no op for the server.
        // If you're trying to join a placement that you have not started, this will be empty and we will attempt to
        // fetch and cache it before recursing into this function again.
    {
        const auto GetDelegate = FQueryMatchPlacementCompletedDelegate::CreateLambda([this, Delegate]
            (bool bSuccess, const FString& MatchPlacementId, const FString& ErrorMessage)
        {
            if (bSuccess)
            {
                JoinMatchPlacement(MatchPlacementId, Delegate);
            }
            else
            {
                UE_LOG(LogDriftMatchPlacement, Log, TEXT("Fetching match placement '%s' failed. Can't join"), *MatchPlacementId);
                (void)Delegate.ExecuteIfBound(false, {}, ErrorMessage);
            }
        });
        return GetPlacement(MatchPlacementID, GetDelegate);
    }

    UE_LOG(LogDriftMatchPlacement, Log, TEXT("Joining match placement '%s'"), *MatchPlacementID);

    FString URL = MatchPlacementsURL;
    if( !URL.EndsWith("/"))
        URL += "/";
    URL +=  MatchPlacementID;
    const auto Request = RequestManager->Post(URL, JsonValue{rapidjson::kObjectType});
    Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
    {
        UE_LOG(LogDriftMatchPlacement, Log, TEXT("Match placement joined"));
        const auto JsonObject = Doc.GetInternalValue()->AsObject();
        FPlayerSessionInfo SessionInfo;
        SessionInfo.Port = JsonObject->GetStringField("Port");
        SessionInfo.IpAddress = JsonObject->GetStringField("IpAddress");
        SessionInfo.PlayerSessionId = JsonObject->GetStringField("PlayerSessionId");
        (void)Delegate.ExecuteIfBound(true, SessionInfo, "");
    });
    Request->OnError.BindLambda([this, Delegate](ResponseContext& Context)
    {
        FString Error;
        Context.errorHandled = GetResponseError(Context, Error);
        ResetCurrentMatchPlacement();
        (void)Delegate.ExecuteIfBound(false, {}, Error);
    });

    return Request->Dispatch();
}

bool FDriftMatchPlacementManager::FetchPublicMatchPlacements(FFetchPublicMatchPlacementsCompletedDelegate Delegate)
{
    if (!HasSession())
    {
        UE_LOG(LogDriftMatchPlacement, Error, TEXT("Trying to query public match placements without a session"));
        (void)Delegate.ExecuteIfBound(false, 0, "No backend connection");
        return false;
    }

    UE_LOG(LogDriftMatchPlacement, Log, TEXT("Querying for public match placements"));

    const auto Request = RequestManager->Get(PublicPlacementsURL);
    Request->OnResponse.BindLambda([this, Delegate](ResponseContext& Context, JsonDocument& Doc)
    {
        UE_LOG(LogDriftMatchPlacement, Verbose, TEXT("FetchPublicMatchPlacements response:'n'%s'"), *Doc.ToString());

        PublicMatchPlacements.Empty();

        const TArray<TSharedPtr<FJsonValue>> *PlacementsJson = nullptr;
        if (Doc.GetInternalValue()->TryGetArray(PlacementsJson))
        {
            for (const auto& PlacementJson : *PlacementsJson)
            {
                FDriftMatchPlacementResponse MatchPlacementResponse{};
                if (!MatchPlacementResponse.FromJson(PlacementJson->AsObject()))
                {
                    UE_LOG(LogDriftMatchPlacement, Error, TEXT("Failed to serialize get match placement response"));
                    return;
                }

                // Only react to Fulfilled as we use these for joining existing games
                const auto Status = ParseStatus(MatchPlacementResponse.Status);
                if (Status == EDriftMatchPlacementStatus::Fulfilled)
                {
                    TSharedPtr<FDriftMatchPlacement> PublicMatchPlacement = MakeShared<FDriftMatchPlacement>(
                        MatchPlacementResponse.PlacementId,
                        MatchPlacementResponse.MapName,
                        MatchPlacementResponse.PlayerId,
                        MatchPlacementResponse.MaxPlayers,
                        ParseStatus(MatchPlacementResponse.Status),
                        MatchPlacementResponse.CustomData,
                        MatchPlacementResponse.MatchPlacementURL
                    );

                    const TArray<TSharedPtr<FJsonValue>> *PlayerIdsJson = nullptr;
                    if (PlacementJson->AsObject()->TryGetArrayField("player_ids", PlayerIdsJson))
                    {
                        for (auto& PlacementPlayerIdValue : *PlayerIdsJson)
                        {
                            int32 PlacementPlayerId = 0;
                            PlacementPlayerIdValue->TryGetNumber(PlacementPlayerId);
                            PublicMatchPlacement->PlayerIds.Add(PlacementPlayerId);
                        }
                    }

                    /*
                    if (!MatchPlacementResponse.ConnectionString.IsEmpty())
                    {
                        PublicMatchPlacement.Get()->ConnectionString = MatchPlacementResponse.ConnectionString;
                        PublicMatchPlacement.Get()->ConnectionOptions = MatchPlacementResponse.ConnectionOptions.IsEmpty() ? "SpectatorOnly=1" : MatchPlacementResponse.ConnectionOptions;
                    }
                    */

                    PublicMatchPlacements.Add(PublicMatchPlacement);
                }
                else
                {
                    UE_LOG(LogDriftMatchPlacement, Log, TEXT("Match placement '%s' found, but the status is '%s'. Ignoring."), *MatchPlacementResponse.PlacementId, *MatchPlacementResponse.Status);
                }
            }
        }
        (void)Delegate.ExecuteIfBound(true, PublicMatchPlacements.Num(), "");

    });
    Request->OnError.BindLambda([Delegate](ResponseContext& Context)
    {
        FString Error;
        Context.errorHandled = GetResponseError(Context, Error);
        (void)Delegate.ExecuteIfBound(false, 0, Error);
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
		case EDriftMatchPlacementStatus::Fulfilled:
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

		    CacheMatchPlacement(EventData);
		    break;
		}

		case EDriftMatchPlacementStatus::Issued:
		case EDriftMatchPlacementStatus::Cancelled:
		case EDriftMatchPlacementStatus::TimedOut:
		case EDriftMatchPlacementStatus::Failed:
		{
		    CacheMatchPlacement(EventData);
		    break;
		}

		case EDriftMatchPlacementStatus::Unknown:
		default:
			UE_LOG(LogDriftMatchPlacement, Error, TEXT("HandleMatchPlacementEvent - Unknown event '%s'. Syncing up the match placement state just in case."), *Event);
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

EDriftMatchPlacementStatus FDriftMatchPlacementManager::ParseStatus(const FString& Status)
{
    if (Status == TEXT("pending")) { return EDriftMatchPlacementStatus::Issued; }
    if (Status == TEXT("completed")) { return EDriftMatchPlacementStatus::Fulfilled; }
    if (Status == TEXT("cancelled")) { return EDriftMatchPlacementStatus::Cancelled; }
    if (Status == TEXT("timed_out")) { return EDriftMatchPlacementStatus::TimedOut; }
    if (Status == TEXT("failed")) { return EDriftMatchPlacementStatus::Failed; }

    return EDriftMatchPlacementStatus::Unknown;
}

bool FDriftMatchPlacementManager::HasSession() const
{
	return !MatchPlacementsURL.IsEmpty() && RequestManager.IsValid();
}

void FDriftMatchPlacementManager::CacheMatchPlacement(const JsonValue& MatchPlacementJsonValue)
{
    FDriftMatchPlacementResponse MatchPlacementResponse{};
    if (!MatchPlacementResponse.FromJson(MatchPlacementJsonValue.GetInternalValue()->AsObject()))
    {
        UE_LOG(LogDriftMatchPlacement, Error, TEXT("Failed to cache match placement. Failed to serialize match placement."));
        return;
    }

    CacheMatchPlacement(MatchPlacementResponse);
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
		ParseStatus(MatchPlacementResponse.Status),
		MatchPlacementResponse.CustomData,
		CurrentMatchPlacementURL
	);

	if (!MatchPlacementResponse.ConnectionString.IsEmpty())
	{
		CurrentMatchPlacement->ConnectionString = MatchPlacementResponse.ConnectionString;
		CurrentMatchPlacement->ConnectionOptions = MatchPlacementResponse.ConnectionOptions.IsEmpty() ? "SpectatorOnly=1" : MatchPlacementResponse.ConnectionOptions;
	}

    UE_LOG(LogDriftMatchPlacement, Verbose, TEXT("Cached match placement '%s' from response '%s'"), *CurrentMatchPlacement->ToString(), *MatchPlacementResponse.ToString());

	UE_LOG(LogDriftMatchPlacement, Log, TEXT("Match placement cached: '%s'"), *CurrentMatchPlacementId);
	OnMatchPlacementStatusChangedDelegate.Broadcast(CurrentMatchPlacementId, CurrentMatchPlacement->MatchPlacementStatus);
}

void FDriftMatchPlacementManager::ResetCurrentMatchPlacement()
{
	CurrentMatchPlacement.Reset();
	CurrentMatchPlacementId.Empty();
	CurrentMatchPlacementURL.Empty();

	UE_LOG(LogDriftMatchPlacement, Log, TEXT("Current match placement state reset"));
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
