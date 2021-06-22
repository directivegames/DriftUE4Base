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

void FDriftFlexmatch::SetRequestManager(TSharedPtr<JsonRequestManager> RootRequestManager)
{
	RequestManager = RootRequestManager;
}

void FDriftFlexmatch::SetEndpoint(const FString& MatchmakingUrl)
{
	FlexmatchURL = MatchmakingUrl;
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
					AverageLatencyMap[Entry.Key] = Entry.Value.GetInt32();
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
		UpdateLocalState();
		IsInitialized = true;
	}
}

void FDriftFlexmatch::StopLatencyReporting()
{
	DoPings = false;
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
			Status = EMatchmakingStatus::None;
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
		Status = EMatchmakingStatus::None;
	});
	Request->Dispatch();
}

EMatchmakingStatus FDriftFlexmatch::GetMatchmakingStatus()
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
	if (Message.sender_id != -1) // FIXME:  define -1 as drift sender 'system'
	{
		UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::HandleMatchmakingEvent - Ignoring message from sender %d"), Message.sender_id);
		return;
	}

	const auto Event = Message.payload.FindField("event").GetString();
	const auto EventData = Message.payload.FindField("event_data");

	UE_LOG(LogDriftMatchmaking, Verbose, TEXT("FDriftFlexmatch::HandleMatchmakingEvent - Incoming event %s"), *Event);
	UpdateLocalState();

	switch (ParseEvent(Event))
	{
		case EMatchmakingEvent::MatchmakingStarted:
			OnMatchmakingStarted().Broadcast();
			break;
		case EMatchmakingEvent::MatchmakingStopped:
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
			const FString MatchId = EventData.FindField("match_id").GetString();
			OnPotentialMatchCreated().Broadcast(PlayersByTeam, MatchId, Requires_Acceptance);
			break;
		}
		case EMatchmakingEvent::MatchmakingSuccess:
		{
			const auto ConnString = EventData.FindField("connection_string").GetString();
			const auto ConnOptions = EventData.FindField("options").GetString();
			OnMatchmakingSuccess().Broadcast({ConnString, ConnOptions});
			break;
		}
		case EMatchmakingEvent::MatchmakingCancelled:
			OnMatchmakingCancelled().Broadcast();
			break;
		case EMatchmakingEvent::AcceptMatch:
		{
			FPlayersAccepted PlayersAccepted;
			for (auto PlayerResponse: EventData.GetObject())
			{
				if (PlayerResponse.Value.GetBool())
				{
					auto PlayerId = FCString::Atoi(*PlayerResponse.Key);
					PlayersAccepted.Add(PlayerId);
				}
			}
			OnAcceptMatch().Broadcast(PlayersAccepted);
			break;
		}
		case EMatchmakingEvent::MatchmakingFailed:
		{
			const auto Reason = EventData.FindField("reason").GetString();
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
		Status = EMatchmakingStatus::Queued;
	else if (StatusString == TEXT("SEARCHING"))
		Status = EMatchmakingStatus::Searching;
	else if (StatusString == TEXT("REQUIRES_ACCEPTANCE"))
		Status = EMatchmakingStatus::RequiresAcceptance;
	else if (StatusString == TEXT("PLACING"))
		Status = EMatchmakingStatus::Placing;
	else if (StatusString == TEXT("COMPLETED"))
		Status = EMatchmakingStatus::Completed;
	else if (StatusString == TEXT("MATCH_COMPLETE"))
		Status = EMatchmakingStatus::MatchCompleted;
	else if (StatusString == TEXT("CANCELLED"))
		Status = EMatchmakingStatus::Cancelled;
	else if (StatusString == TEXT("FAILED"))
		Status = EMatchmakingStatus::Failed;
	else if (StatusString == TEXT("TIMED_OUT"))
		Status = EMatchmakingStatus::TimedOut;
	else
		UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::SetStatusFromString - Unknown status %s - Status not updated"), *StatusString);
}

void FDriftFlexmatch::UpdateLocalState()
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
			Status = EMatchmakingStatus::None;
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
	});
	Request->Dispatch();
}

EMatchmakingEvent FDriftFlexmatch::ParseEvent(const FString& EventName)
{
	if (EventName == TEXT("MatchmakingStarted"))
		return EMatchmakingEvent::MatchmakingStarted;
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
