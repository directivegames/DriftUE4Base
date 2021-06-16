// Copyright 2021 Directive Games Limited - All Rights Reserved

#pragma once

#include "IDriftMatchmaker.h"
#include "DriftMessageQueue.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDriftMatchmaking, Log, All);


class FDriftFlexmatch : public IDriftMatchmaker, FTickableGameObject
{
public:
	FDriftFlexmatch(TSharedPtr<IDriftMessageQueue> MessageQueue_);
	~FDriftFlexmatch() override;

	void SetRequestManager(TSharedPtr<JsonRequestManager> RootRequestManager);
	void SetEndpoint(const FString& MatchmakingUrl);

	// FTickableGameObject overrides
	void Tick(float DeltaTime) override;
	bool IsTickable() const override;
	TStatId GetStatId() const override;

	// IDriftMatchmaker overrides
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
	void ReportLatencies();

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

	// Latency measuring/reporting
	bool DoPings = false;
	const float PingInterval = 7.0;
	float TimeToPing = 0;
	const FString PingUrlTemplate = TEXT("https://gamelift.{0}.amazonaws.com");
	const TArray<FString> PingRegions{"eu-west-1"};

};
