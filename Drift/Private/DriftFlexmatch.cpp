// Copyright 2021 Directive Games Limited - All Rights Reserved

#include "DriftFlexmatch.h"


DEFINE_LOG_CATEGORY(LogDriftMatchmaking);

FDriftFlexmatch::FDriftFlexmatch(TSharedPtr<IDriftMessageQueue> InMessageQueue)
	: MessageQueue{MoveTemp(InMessageQueue)}
{
	MessageQueue->OnMessageQueueMessage(TEXT("matchmaking")).AddRaw(
		this, &FDriftFlexmatch::HandleMatchmakingEvent);
}

FDriftFlexmatch::~FDriftFlexmatch()
{
	MessageQueue->OnMessageQueueMessage(TEXT("matchmaking")).RemoveAll(this);
}

void FDriftFlexmatch::ConfigureSession(TSharedPtr<JsonRequestManager> RootRequestManager, const FString& MatchmakingUrl, int32 InPlayerId)
{
	RequestManager = RootRequestManager;
	FlexmatchURL = MatchmakingUrl;
	PlayerId = InPlayerId;
}

void FDriftFlexmatch::Tick( float DeltaTime )
{
	if ( DoPings )
	{
		TimeToPing -= DeltaTime;
		if (TimeToPing < 0)
		{
			ReportLatencies();
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

void FDriftFlexmatch::ReportLatencies()
{
	const auto HttpModule = &FHttpModule::Get();
	for(auto Region: PingRegions)
	{
		const auto Request = HttpModule->CreateRequest();
		Request->SetVerb("GET");
		Request->SetURL(FString::Format(*PingUrlTemplate, {Region}));
		Request->OnProcessRequestComplete().BindLambda(
		[this, Region](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			if (!bConnectedSuccessfully)
			{
				UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::ReportLatencies - Failed to connect to '%s'"), *Request->GetURL());
				return;
			}
			JsonValue Payload{rapidjson::kObjectType};
			JsonArchive::AddMember(Payload, TEXT("region"), *Region);
			JsonArchive::AddMember(Payload, TEXT("latency_ms"), Request->GetElapsedTime() * 1000);
			auto PatchRequest = RequestManager->Patch(FlexmatchURL, Payload, HttpStatusCodes::Ok);
			PatchRequest->OnError.BindLambda([this](ResponseContext& context)
			{
				UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::ReportLatencies - Failed to report latencies to %s"
					", Response code %d, error: '%s'"), *FlexmatchURL, context.responseCode, *context.error);
			});
			PatchRequest->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
			{
				for( auto Entry: doc.GetObject() )
				{
					AverageLatencyMap.Add(Entry.Key, Entry.Value.GetInt32());
				}
			});
			PatchRequest->Dispatch();
		});
		Request->ProcessRequest();
	}
}

void FDriftFlexmatch::StartLatencyReporting()
{
	DoPings = true;
	if (! IsInitialized )
	{
		InitializeLocalState();
		IsInitialized = true;
	}
}

void FDriftFlexmatch::StopLatencyReporting()
{
	DoPings = false;
	TimeToPing = 0.0;
}

FLatencyMap FDriftFlexmatch::GetLatencyAverages()
{
	return AverageLatencyMap;
}

void FDriftFlexmatch::StartMatchmaking(const FString& MatchmakingConfiguration)
{
	JsonValue Payload{rapidjson::kObjectType};
	JsonArchive::AddMember(Payload, TEXT("matchmaker"), *MatchmakingConfiguration);
	auto Request = RequestManager->Post(FlexmatchURL, Payload, HttpStatusCodes::Ok);
	Request->OnError.BindLambda([this, MatchmakingConfiguration](ResponseContext& context)
	{
		UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::StartMatchmaking - Failed to initiate matchmaking with configuration %s"
					", Response code %d, error: '%s'"), *MatchmakingConfiguration, context.responseCode, *context.error);
	});
	Request->OnResponse.BindLambda([this, MatchmakingConfiguration](ResponseContext& context, JsonDocument& doc)
	{
		const auto TicketId = doc.FindField(TEXT("TicketId")).GetString();
		const auto StatusString = doc.FindField(TEXT("Status")).GetString();
		UE_LOG(LogDriftMatchmaking, Log, TEXT("FDriftFlexmatch::StartMatchmaking - Matchmaking started with configuration %s"
					", TicketId %s, status %s"), *MatchmakingConfiguration, *TicketId, *StatusString);
		SetStatusFromString(StatusString);
	});
	Request->Dispatch();
}

void FDriftFlexmatch::StopMatchmaking()
{
	auto Request = RequestManager->Delete(FlexmatchURL, HttpStatusCodes::NoContent);
	Request->OnError.BindLambda([this](ResponseContext& context)
	{
		if ( context.responseCode == static_cast<int32>(HttpStatusCodes::NotFound) )
		{
			UE_LOG(LogDriftMatchmaking, Verbose, TEXT("FDriftFlexmatch::StopMatchmaking - Server had no active ticket to delete"));
			Status = EMatchmakingTicketStatus::None;
			TicketId.Empty();
		}
		else
		{
			UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::StopMatchmaking - Failed to cancel matchmaking"
					", Response code %d, error: '%s'"), context.responseCode, *context.error);
		}
	});
	Request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
	{
		UE_LOG(LogDriftMatchmaking, Log, TEXT("FDriftFlexmatch::StopMatchmaking - Matchmaking ticket cancelled"));
		Status = EMatchmakingTicketStatus::None;
	});
	Request->Dispatch();
}

EMatchmakingTicketStatus FDriftFlexmatch::GetMatchmakingStatus()
{
	return Status;
}

void FDriftFlexmatch::SetAcceptance(const FString& MatchId, bool Accepted)
{
	JsonValue Payload{rapidjson::kObjectType};
	JsonArchive::AddMember(Payload, TEXT("match_id"), *MatchId);
	JsonArchive::AddMember(Payload, TEXT("acceptance"), Accepted);
	auto Request = RequestManager->Put(FlexmatchURL, Payload, HttpStatusCodes::Ok);
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

FConnectionInfo FDriftFlexmatch::ConnectionInfo() const
{
	return {ConnectionString, ConnectionOptions};
}

void FDriftFlexmatch::HandleMatchmakingEvent(const FMessageQueueEntry& Message)
{
	if (Message.sender_id != 0 && Message.sender_id != PlayerId) // FIXME:  define 0 as drift sender 'system'
	{
		UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::HandleMatchmakingEvent - Ignoring message from sender %d"), Message.sender_id);
		return;
	}

	const auto Event = Message.payload.FindField("event").GetString();
	const auto EventData = Message.payload.FindField("data");

	UE_LOG(LogDriftMatchmaking, Verbose, TEXT("FDriftFlexmatch::HandleMatchmakingEvent - Incoming event %s, local state %s"), *Event, *GetStatusString());

	switch (ParseEvent(Event))
	{
		case EMatchmakingEvent::MatchmakingStarted:
			Status = EMatchmakingTicketStatus::Queued;
			OnMatchmakingStarted().Broadcast();
			break;
		case EMatchmakingEvent::MatchmakingSearching:
			Status = EMatchmakingTicketStatus::Searching;
			OnMatchmakingSearching().Broadcast();
			break;
		case EMatchmakingEvent::MatchmakingStopped:
			Status = EMatchmakingTicketStatus::None;
			OnMatchmakingStopped().Broadcast();
			break;
		case EMatchmakingEvent::PotentialMatchCreated:
		{
			FPlayersByTeam PlayersByTeam;
			for (auto Team: EventData.GetObject())
			{
				for (auto Player: Team.Value.GetArray())
				{
					PlayersByTeam[Team.Key].Add(Player.GetInt32());
				}
			}
			const bool Requires_Acceptance = EventData.FindField("acceptance_required").GetBool();
			Status = Requires_Acceptance ? EMatchmakingTicketStatus::RequiresAcceptance : EMatchmakingTicketStatus::Placing;
			const FString MatchId = EventData.FindField("match_id").GetString();
			OnPotentialMatchCreated().Broadcast(PlayersByTeam, MatchId, Requires_Acceptance);
			break;
		}
		case EMatchmakingEvent::MatchmakingSuccess:
		{
			const auto ConnString = EventData.FindField("connection_string").GetString();
			const auto ConnOptions = EventData.FindField("options").GetString();
			Status = EMatchmakingTicketStatus::Completed;
			OnMatchmakingSuccess().Broadcast({ConnString, ConnOptions});
			break;
		}
		case EMatchmakingEvent::MatchmakingCancelled:
			Status = EMatchmakingTicketStatus::Cancelled;
			OnMatchmakingCancelled().Broadcast();
			break;
		case EMatchmakingEvent::AcceptMatch:
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
			OnAcceptMatch().Broadcast(PlayersAccepted);
			break;
		}
		case EMatchmakingEvent::MatchmakingFailed:
		{
			const auto Reason = EventData.FindField("reason").GetString();
			Status = EMatchmakingTicketStatus::Failed;
			OnMatchmakingFailed().Broadcast(Reason);
			break;
		}
		case EMatchmakingEvent::Unknown:
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
	auto Request = RequestManager->Get(FlexmatchURL, HttpStatusCodes::Ok);
	Request->OnError.BindLambda([this](ResponseContext& context)
	{
		UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::SetCurrentState - Error fetching matchmaking state from server"
					", Response code %d, error: '%s'"), context.responseCode, *context.error);
	});
	Request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
	{
		auto Response = doc.GetObject();
		if ( Response.Num() == 0)
		{
			TicketId.Empty();
			Status = EMatchmakingTicketStatus::None;
			return;
		}
		TicketId = Response["TicketId"].GetString();
		SetStatusFromString(Response["Status"].GetString());
		if ( Response.Contains("GameSessionConnectionInfo") )
		{
			auto SessionInfo = Response["GameSessionConnectionInfo"];
			ConnectionString = SessionInfo.FindField("ConnectionString").GetString();
			ConnectionOptions = SessionInfo.FindField("ConnectionOptions").GetString();
		}
		else
		{
			ConnectionString.Empty();
			ConnectionOptions.Empty();
		}
		switch(Status)
		{
			case EMatchmakingTicketStatus::Queued:
				OnMatchmakingStarted().Broadcast();
				break;
			case EMatchmakingTicketStatus::Searching:
				OnMatchmakingSearching().Broadcast();
				break;
			case EMatchmakingTicketStatus::RequiresAcceptance:
			case EMatchmakingTicketStatus::Placing:
				// FIXME: Dig up players, teams and their acceptance status from the ticket and do the broadcast to allow
				// recovery from disconnects while searching.
				//OnPotentialMatchCreated().Broadcast();
				break;
			case EMatchmakingTicketStatus::Completed:
				OnMatchmakingSuccess().Broadcast({ConnectionString, ConnectionOptions});
				break;
			default:
				break;
		}
	});
	Request->Dispatch();
}

EMatchmakingEvent FDriftFlexmatch::ParseEvent(const FString& EventName)
{
	if (EventName == TEXT("MatchmakingStarted"))
		return EMatchmakingEvent::MatchmakingStarted;
	if (EventName == TEXT("MatchmakingSearching"))
		return EMatchmakingEvent::MatchmakingSearching;
	if (EventName == TEXT("MatchmakingStopped"))
		return EMatchmakingEvent::MatchmakingStopped;
	if (EventName == TEXT("PotentialMatchCreated"))
		return EMatchmakingEvent::PotentialMatchCreated;
	if (EventName == TEXT("MatchmakingSuccess"))
		return EMatchmakingEvent::MatchmakingSuccess;
	if (EventName == TEXT("MatchmakingCancelled"))
		return EMatchmakingEvent::MatchmakingCancelled;
	if (EventName == TEXT("AcceptMatch"))
		return EMatchmakingEvent::AcceptMatch;
	if (EventName == TEXT("MatchmakingFailed"))
		return EMatchmakingEvent::MatchmakingFailed;
	return EMatchmakingEvent::Unknown;

}

FMatchmakingStartedDelegate& FDriftFlexmatch::OnMatchmakingStarted()
{
	return OnMatchmakingStartedDelegate;
}

FMatchmakingSearchingDelegate& FDriftFlexmatch::OnMatchmakingSearching()
{
	return OnMatchmakingSearchingDelegate;
}

FMatchmakingStoppedDelegate& FDriftFlexmatch::OnMatchmakingStopped()
{
	return OnMatchmakingStoppedDelegate;
}

FMatchmakingCancelledDelegate& FDriftFlexmatch::OnMatchmakingCancelled()
{
	return OnMatchmakingCancelledDelegate;
}

FMatchmakingFailedDelegate& FDriftFlexmatch::OnMatchmakingFailed()
{
	return OnMatchmakingFailedDelegate;
}

FPotentialMatchCreatedDelegate& FDriftFlexmatch::OnPotentialMatchCreated()
{
	return OnPotentialMatchCreatedDelegate;
}

FAcceptMatchDelegate& FDriftFlexmatch::OnAcceptMatch()
{
	return OnAcceptMatchDelegate;
}

FMatchmakingSuccessDelegate& FDriftFlexmatch::OnMatchmakingSuccess()
{
	return OnMatchmakingSuccessDelegate;
}
