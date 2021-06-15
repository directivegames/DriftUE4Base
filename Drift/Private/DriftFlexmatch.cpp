// Copyright 2021 Directive Games Limited - All Rights Reserved

#include "DriftFlexmatch.h"
#include "Engine/GameInstance.h"

DEFINE_LOG_CATEGORY(LogDriftMatchmaking);

FDriftFlexmatch::FDriftFlexmatch(TSharedPtr<IDriftMessageQueue> MessageQueue)
	: MessageQueue{MoveTemp(MessageQueue)}
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

void FDriftFlexmatch::StartLatencyReporting()
{
	// Implement
}

void FDriftFlexmatch::StopLatencyReporting()
{
	// Implement
}

TMap<FString, int16> FDriftFlexmatch::GetLatencyAverages()
{
	// Implement
	return {};
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
