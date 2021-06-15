// Copyright 2021 Directive Games Limited - All Rights Reserved

#pragma once

#include "IDriftMatchmaker.h"
#include "DriftMessageQueue.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDriftMatchmaking, Log, All);


class FDriftFlexmatch : public IDriftMatchmaker, public FSelfRegisteringExec
{
public:
	FDriftFlexmatch(TSharedPtr<IDriftMessageQueue> MessageQueue);
	~FDriftFlexmatch() override;
	void SetRequestManager(TSharedPtr<JsonRequestManager> RootRequestManager);
	void SetEndpoint(const FString& MatchmakingUrl);

	void StartLatencyReporting() override;
	void StopLatencyReporting() override;
	TMap<FString, int16> GetLatencyAverages() override;

	void StartMatchmaking() override;
	void StopMatchmaking() override;
	EMatchmakingState MatchmakingStatus() override;

	void SetAcceptance(bool accepted) override;

	FMatchmakingStartedDelegate& OnMatchmakingStarted() override;
	FMatchmakingStoppedDelegate& OnMatchmakingStopped() override;
	FMatchmakingCancelledDelegate& OnMatchmakingCancelled() override;
	FMatchmakingFailedDelegate& OnMatchmakingFailed() override;
	FPotentialMatchCreatedDelegate& OnPotentialMatchCreated() override;
	FAcceptMatchDelegate& OnAcceptMatch() override;
	FMatchmakingSuccessDelegate& OnMatchmakingSuccess() override;

private:
	void HandleMatchmakingEvent(const FMessageQueueEntry& Message);

	TSharedPtr<JsonRequestManager> RequestManager;
	TSharedPtr<IDriftMessageQueue> MessageQueue;
	FString FlexmatchURL;

	FMatchmakingStartedDelegate OnMatchmakingStartedDelegate;
	FMatchmakingStoppedDelegate OnMatchmakingStoppedDelegate;
	FMatchmakingCancelledDelegate OnMatchmakingCancelledDelegate;
	FMatchmakingFailedDelegate OnMatchmakingFailedDelegate;
	FPotentialMatchCreatedDelegate OnPotentialMatchCreatedDelegate;
	FAcceptMatchDelegate OnAcceptMatchDelegate;
	FMatchmakingSuccessDelegate OnMatchmakingSuccessDelegate;
};
