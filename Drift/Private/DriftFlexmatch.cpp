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
		auto TicketId = doc.FindField(TEXT("TicketId")).GetString();
		auto StatusString = doc.FindField(TEXT("Status")).GetString();
		UE_LOG(LogDriftMatchmaking, Log, TEXT("FDriftFlexmatch::StartMatchmaking - Matchmaking started with configuration %s"
					", TicketId %s, status %s"), *MatchmakingConfiguration, *TicketId, *StatusString);
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
	});
	Request->Dispatch();
}

EMatchmakingState FDriftFlexmatch::MatchmakingStatus()
{
	return Status;
}

void FDriftFlexmatch::SetAcceptance(bool accepted)
{
	// Implement
}

void FDriftFlexmatch::HandleMatchmakingEvent(const FMessageQueueEntry& Message)
{
	// Implement
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

void FDriftFlexmatch::SetStatusFromString(const FString& StatusString)
{
	switch (StatusString)
	{
		case "QUEUED":
			Status = EMatchmakingState::Queued;
			break;;
		case "SEARCHING":
			Status = EMatchmakingState::Searching;
			break;
		case "REQUIRES_ACCEPTANCE":
			Status = EMatchmakingState::RequiresAcceptance;
			break;
		case "PLACING":
			Status = EMatchmakingState::Placing;
			break;
		case "COMPLETED":
			Status = EMatchmakingState::Completed;
			break;
		case "MATCH_COMPLETE":
			Status = EMatchmakingState::MatchCompleted;
			break;
		case "CANCELLED":
			Status = EMatchmakingState::Cancelled;
			break;
		case "FAILED":
			Status = EMatchmakingState::Failed;
			break;
		case "TIMED_OUT":
			Status = EMatchmakingState::TimedOut;
			break;
		default:
			UE_LOG(LogDriftMatchmaking, Error, TEXT("FDriftFlexmatch::SetStatusFromString - Unknown status %s - Status not updated"), *StatusString);
	}
}
