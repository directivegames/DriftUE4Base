// Copyright 2021 Directive Games Limited - All Rights Reserved

#include "DriftFlexmatch.h"


DEFINE_LOG_CATEGORY(LogDriftMatchmaking);

static const FString MatchmakingMessageQueue(TEXT("matchmaking"));

FDriftFlexmatch::FDriftFlexmatch(TSharedPtr<IDriftMessageQueue> InMessageQueue)
	: MessageQueue{MoveTemp(InMessageQueue)}
{
	MessageQueue->OnMessageQueueMessage(MatchmakingMessageQueue).AddRaw(
		this, &FDriftFlexmatch::HandleMatchmakingEvent);
}

FDriftFlexmatch::~FDriftFlexmatch()
{
	bDoPings = false;
	MessageQueue->OnMessageQueueMessage(MatchmakingMessageQueue).RemoveAll(this);
}

void FDriftFlexmatch::SetRequestManager(TSharedPtr<JsonRequestManager> RootRequestManager)
{
	RequestManager = RootRequestManager;
}

void FDriftFlexmatch::ConfigureSession(const FDriftEndpointsResponse& DriftEndpoints, int32 InPlayerId)
{
	FlexmatchLatencyURL = DriftEndpoints.my_flexmatch;
	FlexmatchTicketsURL = DriftEndpoints.flexmatch_tickets;
	CurrentTicketUrl = DriftEndpoints.my_flexmatch_ticket;
	PlayerId = InPlayerId;
}

void FDriftFlexmatch::Tick( float DeltaTime )
{
	if ( bDoPings )
	{
		TimeToPing -= DeltaTime;
		if (TimeToPing < 0 && !bIsPinging)
		{
			MeasureLatencies();
			TimeToPing = PingInterval;
		}
	}
}

bool FDriftFlexmatch::IsTickable() const
{
	return true;
}

TStatId FDriftFlexmatch::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FDriftFlexmatch, STATGROUP_Tickables);
}

void FDriftFlexmatch::MeasureLatencies()
{
	bIsPinging = true;
	// Accumulate a mapping of regions->ping and once all results are in, PATCH drift-flexmatch
	auto LatenciesByRegion{ MakeShared<TMap<FString, int>>() };
	const auto HttpModule = &FHttpModule::Get();
	for(auto Region: PingRegions)
	{
		const auto Request = HttpModule->CreateRequest();
		Request->SetVerb("GET");
		Request->SetURL(FString::Format(*PingUrlTemplate, {Region}));
		Request->OnProcessRequestComplete().BindLambda(
		[WeakSelf = TWeakPtr<FDriftFlexmatch>(this->AsShared()), Region, LatenciesByRegion](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			if (auto Self = WeakSelf.Pin())
			{
				if (!bConnectedSuccessfully)
				{
					UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::MeasureLatencies - Failed to connect to '%s'"), *Request->GetURL());
					LatenciesByRegion->Add(Region, -1);
				}
				else
				{
					LatenciesByRegion->Add(Region, static_cast<int>(Request->GetElapsedTime() * 1000));
				}
				// if all regions have been added to the map, report back to drift
				if (LatenciesByRegion->Num() == Self->PingRegions.Num())
				{
					Self->bIsPinging = false;
					if (Self->PingInterval < Self->MaxPingInterval)
					{
						Self->PingInterval += 0.5;
					}
					Self->ReportLatencies(LatenciesByRegion);
				}
			}
		});
		Request->ProcessRequest();
	}
}

void FDriftFlexmatch::ReportLatencies(const TSharedRef<TMap<FString, int>> LatenciesByRegion)
{
	// If we've lost connection, bail
	if ( ! RequestManager )
	{
		return;
	}
	JsonValue LatenciesPayload{rapidjson::kObjectType};
	for (auto entry: *LatenciesByRegion)
	{
		if ( entry.Value == -1 ) // failed ping, skip it from the map
		{
			continue;
		}
		JsonArchive::AddMember(LatenciesPayload, entry.Key, entry.Value);
	}
	if (LatenciesPayload.MemberCount() == 0)
	{
		UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::ReportLatencies - No valid values to report!"));
		return;
	}
	JsonValue PatchPayload{rapidjson::kObjectType};
	JsonArchive::AddMember(PatchPayload, TEXT("latencies"), LatenciesPayload);
	const auto PatchRequest = RequestManager->Patch(FlexmatchLatencyURL, PatchPayload, HttpStatusCodes::Ok);
	PatchRequest->OnError.BindLambda([this](ResponseContext& context)
	{
		UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::ReportLatencies - Failed to report latencies to %s"
			", Response code %d, error: '%s'"), *FlexmatchLatencyURL, context.responseCode, *context.error);
	});
	PatchRequest->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
	{
		FDriftFlexmatchLatencySchema LatencyAverages;
		if (!JsonArchive::LoadObject(doc, LatencyAverages))
		{
			UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::ReportLatencies - Error parsing reponse from PATCHing latencies"
				", Response code %d, error: '%s'"), context.responseCode, *context.error);
			return;
		}
		for( auto Entry: LatencyAverages.latencies.GetObject() )
		{
			AverageLatencyMap.Add(Entry.Key, Entry.Value.GetInt32());
		}
	});
	PatchRequest->Dispatch();
}


void FDriftFlexmatch::StartLatencyReporting()
{
	bDoPings = true;
	if (! bIsInitialized )
	{
		InitializeLocalState();
		bIsInitialized = true;
	}
}

void FDriftFlexmatch::StopLatencyReporting()
{
	bDoPings = false;
	TimeToPing = 0.0;
}

FLatencyMap FDriftFlexmatch::GetLatencyAverages()
{
	return AverageLatencyMap;
}

void FDriftFlexmatch::StartMatchmaking(const FString& MatchmakingConfiguration, const JsonValue& ExtraData)
{
	if (! RequestManager.IsValid() )
	{
		OnDriftMatchmakingFailed().Broadcast(TEXT("No Connection"));
		return;
	}
	JsonValue Payload{rapidjson::kObjectType};
	JsonArchive::AddMember(Payload, TEXT("matchmaker"), *MatchmakingConfiguration);
	if (ExtraData.MemberCount())
	{
		JsonArchive::AddMember(Payload, TEXT("extras"), ExtraData);
	}
	auto Request = RequestManager->Post(FlexmatchTicketsURL, Payload, HttpStatusCodes::Ok);
	Request->OnError.BindLambda([this, MatchmakingConfiguration](ResponseContext& context)
	{
		UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::StartMatchmaking - Failed to initiate matchmaking with configuration %s"
					", Response code %d, error: '%s'"), *MatchmakingConfiguration, context.responseCode, *context.error);
		OnDriftMatchmakingFailed().Broadcast(TEXT("Server Error"));
	});
	Request->OnResponse.BindLambda([this, MatchmakingConfiguration](ResponseContext& context, JsonDocument& doc)
	{
		FDriftFlexmatchTicketPostResponse Response;
		if (!JsonArchive::LoadObject(doc, Response))
		{
			UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::StartMatchmaking - Failed to parse response from POST to %s"
						", Response code %d, error: '%s'"), *FlexmatchTicketsURL, context.responseCode, *context.error);
			OnDriftMatchmakingFailed().Broadcast(TEXT("Server Response Error"));
			return;
		}
		CurrentTicketUrl = Response.ticket_url;
		UE_LOG(LogDriftMatchmaking, Log, TEXT("FDriftFlexmatch::StartMatchmaking - Matchmaking started with configuration %s"
					", TicketId %s, status %s"), *MatchmakingConfiguration, *Response.ticket_id, *Response.ticket_status);
		SetStatusFromString(Response.ticket_status);
	});
	Request->Dispatch();
	Status = EMatchmakingTicketStatus::None;
}

void FDriftFlexmatch::StopMatchmaking()
{
	if (! RequestManager.IsValid() )
	{
		return;
	}
	if ( CurrentTicketUrl.IsEmpty() )
	{
		UE_LOG(LogDriftMatchmaking, Warning, TEXT("FDriftFlexmatch::StopMatchmaking - Cancelling without a known ticket"));
		return;
	}
	auto Request = RequestManager->Delete(CurrentTicketUrl);
	Request->OnError.BindLambda([this](ResponseContext& context)
	{
		UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::StopMatchmaking - Failed to cancel matchmaking"
					", Response code %d, error: '%s'"), context.responseCode, *context.error);
	});
	Request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
	{
		FDriftFlexmatchTicketDeleteResponse Response;
		if (!JsonArchive::LoadObject(doc, Response))
		{
			UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::StopMatchmaking - Failed to parse response from DELETE to %s"
						", Response code %d, error: '%s'"), *CurrentTicketUrl, context.responseCode, *context.error);
			return;
		}
		if (Response.status == TEXT("Deleted") || Response.status == TEXT("NoTicketFound"))
		{
			CurrentTicketUrl.Empty();
			Status = EMatchmakingTicketStatus::None;
			if (Response.status == TEXT("Deleted"))
			{
				UE_LOG(LogDriftMatchmaking, Verbose, TEXT("FDriftFlexmatch::StopMatchmaking - Ticket cancelled."));
			}
		}
		else
		{
			UE_LOG(LogDriftMatchmaking, Verbose, TEXT("FDriftFlexmatch::StopMatchmaking - Ticket is in state '%s' and cannot be cancelled anymore."), *Response.status);
		}
	});
	Request->Dispatch();
}

EMatchmakingTicketStatus FDriftFlexmatch::GetMatchmakingStatus()
{
	return Status;
}

void FDriftFlexmatch::SetAcceptance(const FString& MatchId, bool Accepted)
{
	if (! RequestManager.IsValid() )
	{
		return;
	}
	if (CurrentTicketUrl.IsEmpty())
	{
		UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::SetAcceptance - SetAcceptance called for match %s with client having no ticket URL"), *MatchId);
		return;
	}
	JsonValue Payload{rapidjson::kObjectType};
	JsonArchive::AddMember(Payload, TEXT("match_id"), *MatchId);
	JsonArchive::AddMember(Payload, TEXT("acceptance"), Accepted);
	auto Request = RequestManager->Patch(CurrentTicketUrl, Payload, HttpStatusCodes::Ok);
	Request->OnError.BindLambda([this, MatchId](ResponseContext& context)
	{
		UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::SetAcceptance - Failed to update acceptance for match %s"
					", Response code %d, error: '%s'"), *MatchId, context.responseCode, *context.error);
	});
	Request->OnResponse.BindLambda([this, MatchId, Accepted](ResponseContext& context, JsonDocument& doc)
	{
		UE_LOG(LogDriftMatchmaking, Verbose, TEXT("FDriftFlexmatch::SetAcceptance - Updated acceptance for match %s to %b"), *MatchId, Accepted);
	});
	Request->Dispatch();
}

int32 FDriftFlexmatch::GetLocalPlayerId() const
{
	return PlayerId;
}

FConnectionInfo FDriftFlexmatch::ConnectionInfo() const
{
	return {ConnectionString, ConnectionOptions};
}

void FDriftFlexmatch::HandleMatchmakingEvent(const FMessageQueueEntry& Message)
{
	if (Message.sender_id != FDriftMessageQueue::SenderSystemID && Message.sender_id != PlayerId)
	{
		UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::HandleMatchmakingEvent - Ignoring message from sender %d"), Message.sender_id);
		return;
	}

	const auto Event = Message.payload.FindField("event").GetString();
	const auto EventData = Message.payload.FindField("data");

	UE_LOG(LogDriftMatchmaking, Verbose, TEXT("FDriftFlexmatch::HandleMatchmakingEvent - Incoming event %s, local state %s"), *Event, *GetStatusString());

	switch (ParseEvent(Event))
	{
		case EDriftMatchmakingEvent::MatchmakingStarted:
			Status = EMatchmakingTicketStatus::Queued;
			CurrentTicketUrl = EventData.FindField("ticket_url").GetString();
			OnDriftMatchmakingStarted().Broadcast();
			break;
		case EDriftMatchmakingEvent::MatchmakingSearching:
			Status = EMatchmakingTicketStatus::Searching;
			OnDriftMatchmakingSearching().Broadcast();
			break;
		case EDriftMatchmakingEvent::MatchmakingStopped:
			Status = EMatchmakingTicketStatus::None;
			OnDriftMatchmakingStopped().Broadcast();
			break;
		case EDriftMatchmakingEvent::PotentialMatchCreated:
		{
			const FString MatchId = EventData.FindField("match_id").GetString();
			const bool Requires_Acceptance = EventData.FindField("acceptance_required").GetBool();
			Status = Requires_Acceptance ? EMatchmakingTicketStatus::RequiresAcceptance : EMatchmakingTicketStatus::Placing;
			int32 TimeOut = -1;
			if (Requires_Acceptance)
				TimeOut = EventData.FindField("acceptance_timeout").GetInt32();
			FPlayersByTeam PlayersByTeam;
			if (EventData.HasField("teams"))
			{
				for (auto Team: EventData.FindField("teams").GetObject())
				{
					TArray<int32> TeamPlayers;
					for (auto Player: Team.Value.GetArray())
					{
						TeamPlayers.Add(Player.GetInt32());
					}
					PlayersByTeam.Add(Team.Key, TeamPlayers);
				}
			}
			OnDriftPotentialMatchCreated().Broadcast(PlayersByTeam, MatchId, Requires_Acceptance, TimeOut);
			break;
		}
		case EDriftMatchmakingEvent::MatchmakingSuccess:
		{
			ConnectionString = EventData.FindField("connection_string").GetString();
			ConnectionOptions = EventData.FindField("options").GetString();
			Status = EMatchmakingTicketStatus::Completed;
			OnDriftMatchmakingSuccess().Broadcast(ConnectionInfo());
			break;
		}
		case EDriftMatchmakingEvent::MatchmakingCancelled:
			Status = EMatchmakingTicketStatus::Cancelled;
			OnDriftMatchmakingCancelled().Broadcast();
			break;
		case EDriftMatchmakingEvent::AcceptMatch:
		{
			FPlayersAccepted PlayersAccepted;
			for (auto PlayerResponse: EventData.GetObject())
			{
				if (PlayerResponse.Value.GetBool())
				{
					auto AcceptedPlayerId = FCString::Atoi(*PlayerResponse.Key);
					PlayersAccepted.Add(AcceptedPlayerId);
				}
			}
			OnDriftAcceptMatch().Broadcast(PlayersAccepted);
			break;
		}
		case EDriftMatchmakingEvent::MatchmakingFailed:
		{
			const auto Reason = EventData.FindField("reason").GetString();
			Status = EMatchmakingTicketStatus::Failed;
			OnDriftMatchmakingFailed().Broadcast(Reason);
			break;
		}
		case EDriftMatchmakingEvent::Unknown:
		default:
			UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::HandleMatchmakingEvent - Unknown event %s"), *Event);
	}
}

void FDriftFlexmatch::SetStatusFromString(const FString& StatusString)
{
	if (StatusString == TEXT("QUEUED"))
		Status = EMatchmakingTicketStatus::Queued;
	else if (StatusString == TEXT("SEARCHING"))
		Status = EMatchmakingTicketStatus::Searching;
	else if (StatusString == TEXT("REQUIRES_ACCEPTANCE"))
		Status = EMatchmakingTicketStatus::RequiresAcceptance;
	else if (StatusString == TEXT("PLACING"))
		Status = EMatchmakingTicketStatus::Placing;
	else if (StatusString == TEXT("COMPLETED"))
		Status = EMatchmakingTicketStatus::Completed;
	else if (StatusString == TEXT("MATCH_COMPLETE"))
		Status = EMatchmakingTicketStatus::MatchCompleted;
	else if (StatusString == TEXT("CANCELLED"))
		Status = EMatchmakingTicketStatus::Cancelled;
	else if (StatusString == TEXT("FAILED"))
		Status = EMatchmakingTicketStatus::Failed;
	else if (StatusString == TEXT("TIMED_OUT"))
		Status = EMatchmakingTicketStatus::TimedOut;
	else
		UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::SetStatusFromString - Unknown status %s - Status not updated"), *StatusString);
}

FString FDriftFlexmatch::GetStatusString() const
{
	if (Status == EMatchmakingTicketStatus::Queued)
		return TEXT("QUEUED");
	if (Status == EMatchmakingTicketStatus::Searching)
		return TEXT("SEARCHING");
	if (Status == EMatchmakingTicketStatus::RequiresAcceptance)
		return TEXT("REQUIRES_ACCEPTANCE");
	if (Status == EMatchmakingTicketStatus::Placing)
		return TEXT("PLACING");
	if (Status == EMatchmakingTicketStatus::Completed)
		return TEXT("COMPLETED");
	if (Status == EMatchmakingTicketStatus::MatchCompleted)
		return TEXT("MATCH_COMPLETE");
	if (Status == EMatchmakingTicketStatus::Cancelled)
		return TEXT("CANCELLED");
	if (Status == EMatchmakingTicketStatus::Failed)
		return TEXT("FAILED");
	if (Status == EMatchmakingTicketStatus::TimedOut)
		return TEXT("TIMED_OUT");
	return TEXT("");
}

void FDriftFlexmatch::InitializeLocalState()
{
	if (! RequestManager.IsValid() )
	{
		return;
	}
	if (CurrentTicketUrl.IsEmpty())
	{
		return;
	}
	auto Request = RequestManager->Get(CurrentTicketUrl, HttpStatusCodes::Ok);
	Request->OnError.BindLambda([this](ResponseContext& context)
	{
		UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::InitializeLocalState - Error fetching existing ticket"
					", Response code %d, error: '%s'"), context.responseCode, *context.error);
		CurrentTicketUrl.Empty();
	});
	Request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
	{
		auto Response = doc.GetObject();
		if ( Response.Num() == 0)
		{
			CurrentTicketUrl.Empty();
			Status = EMatchmakingTicketStatus::None;
			return;
		}
		auto TicketId = Response["TicketId"].GetString();
		SetStatusFromString(Response["Status"].GetString());
		if ( Response.Contains("GameSessionConnectionInfo") )
		{
			auto SessionInfo = Response["GameSessionConnectionInfo"];
			ConnectionString = SessionInfo.FindField("ConnectionString").GetString();
			ConnectionOptions = SessionInfo.FindField("ConnectionOptions").GetString();
		}
		switch(Status)
		{
			case EMatchmakingTicketStatus::Queued:
				OnDriftMatchmakingStarted().Broadcast();
				break;
			case EMatchmakingTicketStatus::Searching:
				OnDriftMatchmakingSearching().Broadcast();
				break;
			case EMatchmakingTicketStatus::RequiresAcceptance:
			case EMatchmakingTicketStatus::Placing:
			{
				// FIXME: The below is probably buggy as hell but is intended to facilitate recovery from disconnects
				// while searching.
				// We're missing the acceptance status per player as that isn't currently stored in the ticket we just fetched.
				// However, chances are the player will receive a proper event just a tad later which should correct
				// the situation
				FString PotentialMatchId = Response["MatchId"].GetString();
				int32 FakeTimeOut = 10;
				TArray<FString> FakeTeams = {TEXT("Team 1"), TEXT("Team 2")};
				FPlayersByTeam FakeTeamAllocation;
				FakeTeamAllocation.Add(FakeTeams[0], TArray<int32>());
				FakeTeamAllocation.Add(FakeTeams[1], TArray<int32>());
				int TeamIndex = 0;
				for (auto PlayerEntry: Response["Players"].GetArray())
				{
					int32 EntryPlayerId = FCString::Atoi(*PlayerEntry.GetObject()["PlayerId"].GetString());
					FakeTeamAllocation[FakeTeams[TeamIndex++ % 2]].Add(EntryPlayerId);
				}
				OnDriftPotentialMatchCreated().Broadcast(FakeTeamAllocation, PotentialMatchId, Status == EMatchmakingTicketStatus::RequiresAcceptance, FakeTimeOut);
				break;
			}
			case EMatchmakingTicketStatus::Completed:
				OnDriftMatchmakingSuccess().Broadcast({ConnectionString, ConnectionOptions});
				break;
			default:
				break;
		}
	});
	Request->Dispatch();
}

EDriftMatchmakingEvent FDriftFlexmatch::ParseEvent(const FString& EventName)
{
	if (EventName == TEXT("MatchmakingStarted"))
		return EDriftMatchmakingEvent::MatchmakingStarted;
	if (EventName == TEXT("MatchmakingSearching"))
		return EDriftMatchmakingEvent::MatchmakingSearching;
	if (EventName == TEXT("MatchmakingStopped"))
		return EDriftMatchmakingEvent::MatchmakingStopped;
	if (EventName == TEXT("PotentialMatchCreated"))
		return EDriftMatchmakingEvent::PotentialMatchCreated;
	if (EventName == TEXT("MatchmakingSuccess"))
		return EDriftMatchmakingEvent::MatchmakingSuccess;
	if (EventName == TEXT("MatchmakingCancelled"))
		return EDriftMatchmakingEvent::MatchmakingCancelled;
	if (EventName == TEXT("AcceptMatch"))
		return EDriftMatchmakingEvent::AcceptMatch;
	if (EventName == TEXT("MatchmakingFailed"))
		return EDriftMatchmakingEvent::MatchmakingFailed;
	return EDriftMatchmakingEvent::Unknown;

}

FMatchmakingStartedDelegate& FDriftFlexmatch::OnDriftMatchmakingStarted()
{
	return OnMatchmakingStartedDelegate;
}

FMatchmakingSearchingDelegate& FDriftFlexmatch::OnDriftMatchmakingSearching()
{
	return OnMatchmakingSearchingDelegate;
}

FMatchmakingStoppedDelegate& FDriftFlexmatch::OnDriftMatchmakingStopped()
{
	return OnMatchmakingStoppedDelegate;
}

FMatchmakingCancelledDelegate& FDriftFlexmatch::OnDriftMatchmakingCancelled()
{
	return OnMatchmakingCancelledDelegate;
}

FMatchmakingFailedDelegate& FDriftFlexmatch::OnDriftMatchmakingFailed()
{
	return OnMatchmakingFailedDelegate;
}

FPotentialMatchCreatedDelegate& FDriftFlexmatch::OnDriftPotentialMatchCreated()
{
	return OnPotentialMatchCreatedDelegate;
}

FAcceptMatchDelegate& FDriftFlexmatch::OnDriftAcceptMatch()
{
	return OnAcceptMatchDelegate;
}

FMatchmakingSuccessDelegate& FDriftFlexmatch::OnDriftMatchmakingSuccess()
{
	return OnMatchmakingSuccessDelegate;
}
