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
		TimeToFetch -= DeltaTime;
		if (TimeToFetch < 0)
		{
			FetchAverages();
			TimeToFetch = FetchInterval;
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
			PatchRequest->Dispatch();
		});
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

void FDriftFlexmatch::FetchAverages()
{
	auto Request = RequestManager->Get(FlexmatchURL);
	Request->OnResponse.BindLambda([this](ResponseContext& context, JsonDocument& doc)
	{
		for( auto Entry: doc.GetObject() )
		{
			AverageLatencyMap[Entry.Key] = Entry.Value.GetInt32();
		}
	});
}

void FDriftFlexmatch::StartMatchmaking()
{
	// Implement
}

void FDriftFlexmatch::StopMatchmaking()
{
	// Implement
}

EMatchmakingState FDriftFlexmatch::MatchmakingStatus()
{
	// Implement
	return EMatchmakingState::None;
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
